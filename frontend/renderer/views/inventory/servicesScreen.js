/**
 * @file   servicesScreen.js
 * @brief  The Services tab: installed services with state and start type, filterable by text and state.
 *         Refreshes every 5 s. Shows a banner when the platform cannot list services.
 */
import { createPollingScreen } from '../../live/pollingScreen.js';
import { createTableScreen } from '../../ui/tableScreen.js';
import { getServices } from '../../api/system/inventoryApi.js';
import { NO_VALUE, formatInteger } from '../../ui/format.js';

const REFRESH_INTERVAL_MS = 5000;
const STATE_ALL = 'all';

const TEXTS = Object.freeze({
  title: 'Services',
  searchPlaceholder: 'Filter by name or description',
  emptyText: 'No service to show',
  stateFilter: 'Status',
  states: { running: 'Running', stopped: 'Stopped', starting: 'Starting', stopping: 'Stopping', paused: 'Paused', other: 'Other' },
  startTypes: { automatic: 'Automatic', manual: 'Manual', disabled: 'Disabled', static: 'Static', other: 'Other' },
});

/**
 * Create the Services screen definition for the ViewSwitcher.
 * @param {object} options
 * @param {() => Promise<import('../../api/apiClient.js').ApiClient>} options.getClient
 * @returns {{render: Function, onShow: Function, onHide: Function}}
 */
export function createServicesScreen({ getClient }) {
  return createPollingScreen({
    getClient,
    intervalMs: REFRESH_INTERVAL_MS,
    load: getServices,
    create: (host) => {
      const table = createTableScreen({
        title: TEXTS.title,
        searchText: (s) => `${s.name} ${s.displayName} ${s.description ?? ''}`,
        searchPlaceholder: TEXTS.searchPlaceholder,
        emptyText: TEXTS.emptyText,
        defaultSort: { key: 'name', direction: 'asc' },
        rowKey: (s) => s.name,
        filters: [{
          label: TEXTS.stateFilter,
          options: [
            { value: STATE_ALL, label: 'All states' },
            { value: 'running', label: TEXTS.states.running },
            { value: 'stopped', label: TEXTS.states.stopped },
          ],
          predicate: (service, value) => value === STATE_ALL || service.state === value,
        }],
        summary: (shown, all) => `${formatInteger(shown.length)} of ${formatInteger(all.length)} services`,
        rowClass: (s) => `service--${s.state}`,
        columns: [
          { key: 'name', label: 'Name', width: '240px', sortValue: (s) => s.name.toLowerCase(), text: (s) => s.name },
          { key: 'description', label: 'Description', sortValue: (s) => (s.description ?? s.displayName).toLowerCase(),
            text: (s) => s.description ?? s.displayName, className: () => 'table__cell--ellipsis' },
          { key: 'state', label: 'Status', width: '110px', sortValue: (s) => s.state, text: (s) => TEXTS.states[s.state] ?? s.state },
          { key: 'startType', label: 'Startup type', width: '120px', sortValue: (s) => s.startType, text: (s) => TEXTS.startTypes[s.startType] ?? s.startType },
          { key: 'pid', label: 'PID', align: 'right', width: '80px', sortValue: (s) => s.pid ?? -1, text: (s) => (s.pid === null ? NO_VALUE : String(s.pid)) },
        ],
      });
      host.replaceChildren(table.element);
      return {
        update: (data) => table.update({ rows: data.services, unavailableReason: data.available ? null : data.reason }),
      };
    },
  });
}
