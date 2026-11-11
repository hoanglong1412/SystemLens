/**
 * @file   appHistoryScreen.js
 * @brief  The App history tab: CPU time and memory per application name, accumulated since the backend
 *         started (kept in memory only). Refreshes every 2 s.
 */
import { createPollingScreen } from '../../live/pollingScreen.js';
import { createTableScreen } from '../../ui/tableScreen.js';
import { getAppHistory } from '../../api/processes/processApi.js';
import { formatBytes, formatCpuTime, formatDateTime, formatInteger } from '../../ui/format.js';

const REFRESH_INTERVAL_MS = 2000;

const TEXTS = Object.freeze({
  title: 'App history',
  searchPlaceholder: 'Filter by name',
  sincePrefix: 'Since',
  emptyText: 'No application matches the filter',
});

/**
 * Create the App history screen definition for the ViewSwitcher.
 * @param {object} options
 * @param {() => Promise<import('../../api/apiClient.js').ApiClient>} options.getClient
 * @returns {{render: Function, onShow: Function, onHide: Function}}
 */
export function createAppHistoryScreen({ getClient }) {
  return createPollingScreen({
    getClient,
    intervalMs: REFRESH_INTERVAL_MS,
    load: getAppHistory,
    create: (host) => {
      const table = createTableScreen({
        title: TEXTS.title,
        searchText: (app) => app.name,
        searchPlaceholder: TEXTS.searchPlaceholder,
        emptyText: TEXTS.emptyText,
        defaultSort: { key: 'cpuTime', direction: 'desc' },
        rowKey: (app) => app.name,
        summary: (shown, all) => `${formatInteger(shown.length)} of ${formatInteger(all.length)} applications`,
        columns: [
          { key: 'name', label: 'Name', sortValue: (a) => a.name.toLowerCase(), text: (a) => a.name },
          { key: 'cpuTime', label: 'CPU time', align: 'right', width: '120px', sortValue: (a) => a.cpuTimeSeconds, text: (a) => formatCpuTime(a.cpuTimeSeconds) },
          { key: 'peak', label: 'Peak memory', align: 'right', width: '130px', sortValue: (a) => a.peakMemoryBytes, text: (a) => formatBytes(a.peakMemoryBytes) },
          { key: 'memory', label: 'Memory now', align: 'right', width: '130px', sortValue: (a) => a.memoryBytes, text: (a) => formatBytes(a.memoryBytes) },
          { key: 'instances', label: 'Running', align: 'right', width: '90px', sortValue: (a) => a.runningInstances, text: (a) => formatInteger(a.runningInstances) },
        ],
      });
      host.replaceChildren(table.element);
      return {
        update: (history) => table.update({
          rows: history.apps,
          note: `${TEXTS.sincePrefix} ${formatDateTime(history.since, { seconds: true })}`,
        }),
      };
    },
  });
}
