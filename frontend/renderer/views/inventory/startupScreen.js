/**
 * @file   startupScreen.js
 * @brief  The Startup tab: programs started at logon (read-only). Refreshes every 5 s. Shows a banner when
 *         the platform cannot list them.
 */
import { createPollingScreen } from '../../live/pollingScreen.js';
import { createTableScreen } from '../../ui/tableScreen.js';
import { getStartupItems } from '../../api/system/inventoryApi.js';
import { formatInteger } from '../../ui/format.js';

const REFRESH_INTERVAL_MS = 5000;

const TEXTS = Object.freeze({
  title: 'Startup',
  searchPlaceholder: 'Filter by name or command',
  emptyText: 'No startup item found',
  scopes: { user: 'Current user', allUsers: 'All users' },
  enabled: 'Enabled',
  disabled: 'Disabled',
});

/**
 * Create the Startup screen definition for the ViewSwitcher.
 * @param {object} options
 * @param {() => Promise<import('../../api/apiClient.js').ApiClient>} options.getClient
 * @returns {{render: Function, onShow: Function, onHide: Function}}
 */
export function createStartupScreen({ getClient }) {
  return createPollingScreen({
    getClient,
    intervalMs: REFRESH_INTERVAL_MS,
    load: getStartupItems,
    create: (host) => {
      const table = createTableScreen({
        title: TEXTS.title,
        searchText: (item) => `${item.name} ${item.command}`,
        searchPlaceholder: TEXTS.searchPlaceholder,
        emptyText: TEXTS.emptyText,
        defaultSort: { key: 'name', direction: 'asc' },
        rowKey: (item) => `${item.location}|${item.name}`,
        rowClass: (item) => (item.enabled ? '' : 'row--disabled'),
        summary: (shown, all) => `${formatInteger(shown.length)} of ${formatInteger(all.length)} items`,
        columns: [
          { key: 'name', label: 'Name', width: '220px', sortValue: (i) => i.name.toLowerCase(), text: (i) => i.name },
          { key: 'command', label: 'Command', sortValue: (i) => i.command.toLowerCase(), text: (i) => i.command, className: () => 'table__cell--ellipsis' },
          { key: 'scope', label: 'Applies to', width: '120px', sortValue: (i) => i.scope, text: (i) => TEXTS.scopes[i.scope] ?? i.scope },
          { key: 'status', label: 'Status', width: '90px', sortValue: (i) => (i.enabled ? 0 : 1), text: (i) => (i.enabled ? TEXTS.enabled : TEXTS.disabled) },
          { key: 'location', label: 'Location', width: '320px', sortValue: (i) => i.location.toLowerCase(), text: (i) => i.location, className: () => 'table__cell--ellipsis' },
        ],
      });
      host.replaceChildren(table.element);
      return {
        update: (data) => table.update({ rows: data.items, unavailableReason: data.available ? null : data.reason }),
      };
    },
  });
}
