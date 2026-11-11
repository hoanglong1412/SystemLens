/**
 * @file   detailsScreen.js
 * @brief  The Details tab: one flat row per process with the full set of columns (status, owner, CPU time,
 *         parent, command line). Sortable by every column and filterable by text. Refreshes every 2 s.
 */
import { createPollingScreen } from '../../live/pollingScreen.js';
import { createTableScreen } from '../../ui/tableScreen.js';
import { getProcesses } from '../../api/processes/processApi.js';
import { NO_VALUE, formatBytes, formatCpuTime, formatInteger } from '../../ui/format.js';

const REFRESH_INTERVAL_MS = 2000;

const TEXTS = Object.freeze({
  title: 'Details',
  searchPlaceholder: 'Filter by name, PID, user or command line',
  emptyText: 'No process matches the filter',
});

/**
 * Create the Details screen definition for the ViewSwitcher.
 * @param {object} options
 * @param {() => Promise<import('../../api/apiClient.js').ApiClient>} options.getClient
 * @returns {{render: Function, onShow: Function, onHide: Function}}
 */
export function createDetailsScreen({ getClient }) {
  return createPollingScreen({
    getClient,
    intervalMs: REFRESH_INTERVAL_MS,
    load: getProcesses,
    create: (host) => {
      const table = createTableScreen({
        title: TEXTS.title,
        searchText: (p) => `${p.name} ${p.pid} ${p.userName ?? ''} ${p.commandLine ?? ''}`,
        searchPlaceholder: TEXTS.searchPlaceholder,
        emptyText: TEXTS.emptyText,
        defaultSort: { key: 'cpu', direction: 'desc' },
        rowKey: (p) => String(p.pid),
        rowClass: (p) => (p.isSelf ? 'row--self' : ''),
        summary: (shown, all) => `${formatInteger(shown.length)} of ${formatInteger(all.length)} processes`,
        columns: [
          { key: 'pid', label: 'PID', align: 'right', width: '72px', sortValue: (p) => p.pid, text: (p) => String(p.pid) },
          { key: 'name', label: 'Name', width: '190px', sortValue: (p) => p.name.toLowerCase(), text: (p) => p.name },
          { key: 'status', label: 'Status', width: '100px', sortValue: (p) => p.status ?? '', text: (p) => p.status ?? NO_VALUE },
          { key: 'user', label: 'User', width: '150px', sortValue: (p) => p.userName ?? '', text: (p) => p.userName ?? NO_VALUE },
          { key: 'cpu', label: 'CPU %', align: 'right', width: '72px', sortValue: (p) => p.cpuPercent, text: (p) => p.cpuPercent.toFixed(1) },
          { key: 'cpuTime', label: 'CPU time', align: 'right', width: '90px', sortValue: (p) => p.cpuTimeSeconds, text: (p) => formatCpuTime(p.cpuTimeSeconds) },
          { key: 'memory', label: 'Memory', align: 'right', width: '90px', sortValue: (p) => p.memoryBytes, text: (p) => formatBytes(p.memoryBytes) },
          { key: 'threads', label: 'Threads', align: 'right', width: '72px', sortValue: (p) => p.threadCount ?? -1, text: (p) => formatInteger(p.threadCount) },
          { key: 'parent', label: 'Parent PID', align: 'right', width: '88px', sortValue: (p) => p.parentPid, text: (p) => String(p.parentPid) },
          { key: 'command', label: 'Command line', sortValue: (p) => p.commandLine ?? '', text: (p) => p.commandLine ?? NO_VALUE,
            className: () => 'table__cell--ellipsis' },
        ],
      });
      host.replaceChildren(table.element);
      return { update: (processes) => table.update({ rows: processes }) };
    },
  });
}
