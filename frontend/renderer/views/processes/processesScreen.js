/**
 * @file   processesScreen.js
 * @brief  The Processes tab: processes grouped by name. A group row sums the CPU, memory and threads of its
 *         instances and expands to show them; a name with one instance is a plain row. Refreshes every 2 s.
 */
import { h } from '../../ui/dom.js';
import { createPollingScreen } from '../../live/pollingScreen.js';
import { createTableScreen } from '../../ui/tableScreen.js';
import { compareValues } from '../../ui/dataTable.js';
import { getProcesses } from '../../api/processes/processApi.js';
import { NO_VALUE, formatBytes, formatInteger } from '../../ui/format.js';

const REFRESH_INTERVAL_MS = 2000;
const SORT_ASCENDING = 'asc';
/** User shown for a group whose instances belong to different users. */
const MIXED_USERS = 'Various';
const CARET_COLLAPSED = '▸';
const CARET_EXPANDED = '▾';

const TEXTS = Object.freeze({
  title: 'Processes',
  searchPlaceholder: 'Filter by name, PID or user',
  columns: { name: 'Name', pid: 'PID', user: 'User', cpu: 'CPU %', memory: 'Memory', threads: 'Threads' },
  emptyText: 'No process matches the filter',
});

/** Names of the groups the user expanded; kept across refreshes. */
const expandedGroups = new Set();

/**
 * Group the processes by name.
 * @param {object[]} processes ProcessEntry[] from the API.
 * @returns {Map<string, object[]>} Instances per process name, in the order of the input.
 */
function groupByName(processes) {
  const groups = new Map();
  for (const process of processes) {
    if (!groups.has(process.name)) groups.set(process.name, []);
    groups.get(process.name).push(process);
  }
  return groups;
}

/**
 * Row of one process.
 * @param {object} process ProcessEntry.
 * @param {number} depth 0 = top level, 1 = instance inside a group.
 * @returns {object} Display row.
 */
function processRow(process, depth) {
  return {
    key: `p${process.pid}`,
    kind: 'process',
    depth,
    name: process.name,
    pid: process.pid,
    userName: process.userName,
    cpuPercent: process.cpuPercent,
    memoryBytes: process.memoryBytes,
    threadCount: process.threadCount,
    isSelf: process.isSelf,
  };
}

/**
 * Row of a group of several instances: sums of the instances.
 * @param {string} name Process name.
 * @param {object[]} items ProcessEntry[] of the group.
 * @returns {object} Display row.
 */
function groupRow(name, items) {
  const users = new Set(items.map((item) => item.userName));
  return {
    key: `g:${name}`,
    kind: 'group',
    depth: 0,
    name,
    count: items.length,
    pid: null,
    userName: users.size === 1 ? [...users][0] : MIXED_USERS,
    cpuPercent: items.reduce((sum, item) => sum + item.cpuPercent, 0),
    memoryBytes: items.reduce((sum, item) => sum + item.memoryBytes, 0),
    threadCount: items.some((item) => item.threadCount === null)
      ? null
      : items.reduce((sum, item) => sum + item.threadCount, 0),
    isSelf: items.some((item) => item.isSelf),
  };
}

/**
 * Value a column sorts by.
 * @param {object} row Display row.
 * @param {string} key Column key.
 * @returns {number|string}
 */
function sortValueOf(row, key) {
  switch (key) {
    case 'pid': return row.pid ?? -1;
    case 'user': return row.userName ?? '';
    case 'cpu': return row.cpuPercent;
    case 'memory': return row.memoryBytes;
    case 'threads': return row.threadCount ?? -1;
    default: return row.name.toLowerCase();
  }
}

/**
 * Filter, group, sort and flatten the process list (replaces the default arrangement of the table screen).
 * Groups are sorted by their sums, instances inside a group by the same column.
 * @param {object[]} processes ProcessEntry[] from the API.
 * @param {{sortKey: string, sortDirection: string, search: string}} view Sort and filter state.
 * @returns {object[]} Display rows in screen order.
 */
function arrange(processes, view) {
  const searching = view.search !== '';
  const matching = searching
    ? processes.filter((p) => `${p.name} ${p.pid} ${p.userName ?? ''}`.toLowerCase().includes(view.search))
    : processes;
  const direction = view.sortDirection === SORT_ASCENDING ? 1 : -1;
  const bySort = (a, b) => direction * compareValues(sortValueOf(a, view.sortKey), sortValueOf(b, view.sortKey));

  const tops = [...groupByName(matching)].map(([name, items]) => ({
    name,
    items,
    row: items.length === 1 ? processRow(items[0], 0) : groupRow(name, items),
  }));
  tops.sort((a, b) => bySort(a.row, b.row));

  const rows = [];
  for (const { name, items, row } of tops) {
    rows.push(row);
    // While filtering, groups open so the matching instances are visible.
    if (row.kind === 'group' && (searching || expandedGroups.has(name))) {
      rows.push(...items.map((item) => processRow(item, 1)).sort(bySort));
    }
  }
  return rows;
}

/**
 * Name cell: caret for groups, indent for instances.
 * @param {object} row Display row.
 * @returns {HTMLElement}
 */
function nameCell(row) {
  const caret = row.kind === 'group'
    ? h('span', { class: 'process__caret', text: expandedGroups.has(row.name) ? CARET_EXPANDED : CARET_COLLAPSED })
    : h('span', { class: 'process__caret process__caret--none' });
  const label = row.kind === 'group' ? `${row.name} (${row.count})` : row.name;
  return h('span', { class: `process__name process__name--depth${row.depth}` }, [caret, label]);
}

/**
 * Create the Processes screen definition for the ViewSwitcher.
 * @param {object} options
 * @param {() => Promise<import('../../api/apiClient.js').ApiClient>} options.getClient
 * @returns {{render: Function, onShow: Function, onHide: Function}}
 */
export function createProcessesScreen({ getClient }) {
  return createPollingScreen({
    getClient,
    intervalMs: REFRESH_INTERVAL_MS,
    load: getProcesses,
    create: (host) => {
      const table = createTableScreen({
        title: TEXTS.title,
        searchText: () => '',
        searchPlaceholder: TEXTS.searchPlaceholder,
        emptyText: TEXTS.emptyText,
        defaultSort: { key: 'cpu', direction: 'desc' },
        rowKey: (row) => row.key,
        rowClass: (row) => (row.isSelf ? 'row--self' : ''),
        transform: arrange,
        summary: (shown, all) => `${formatInteger(all.length)} processes`,
        onRowClick: (row, { rerender }) => {
          if (row.kind !== 'group') return;
          if (!expandedGroups.delete(row.name)) expandedGroups.add(row.name);
          rerender();
        },
        columns: [
          { key: 'name', label: TEXTS.columns.name, sortValue: (r) => sortValueOf(r, 'name'), render: nameCell },
          { key: 'pid', label: TEXTS.columns.pid, align: 'right', width: '80px', sortValue: (r) => sortValueOf(r, 'pid'),
            text: (r) => (r.pid === null ? '' : String(r.pid)) },
          { key: 'user', label: TEXTS.columns.user, width: '180px', sortValue: (r) => sortValueOf(r, 'user'),
            text: (r) => r.userName ?? NO_VALUE },
          { key: 'cpu', label: TEXTS.columns.cpu, align: 'right', width: '90px', sortValue: (r) => sortValueOf(r, 'cpu'),
            text: (r) => r.cpuPercent.toFixed(1) },
          { key: 'memory', label: TEXTS.columns.memory, align: 'right', width: '110px', sortValue: (r) => sortValueOf(r, 'memory'),
            text: (r) => formatBytes(r.memoryBytes) },
          { key: 'threads', label: TEXTS.columns.threads, align: 'right', width: '80px', sortValue: (r) => sortValueOf(r, 'threads'),
            text: (r) => formatInteger(r.threadCount) },
        ],
      });
      host.replaceChildren(table.element);
      return { update: (processes) => table.update({ rows: processes }) };
    },
  });
}
