/**
 * @file   usersScreen.js
 * @brief  The Users tab: logged-on sessions and, below, CPU / memory used by the processes of each user.
 *         Refreshes every 5 s. Shows a banner when the platform cannot list sessions.
 */
import { h } from '../../ui/dom.js';
import { createPollingScreen } from '../../live/pollingScreen.js';
import { createTableScreen } from '../../ui/tableScreen.js';
import { getUsers } from '../../api/system/inventoryApi.js';
import { NO_VALUE, formatBytes, formatCount, formatDateTime, formatInteger } from '../../ui/format.js';

const REFRESH_INTERVAL_MS = 5000;

const TEXTS = Object.freeze({
  sessionsTitle: 'Sessions',
  usageTitle: 'Resource usage by user',
  sessionsEmpty: 'No user session found',
  usageEmpty: 'No process found',
});

/**
 * Create the Users screen definition for the ViewSwitcher.
 * @param {object} options
 * @param {() => Promise<import('../../api/apiClient.js').ApiClient>} options.getClient
 * @returns {{render: Function, onShow: Function, onHide: Function}}
 */
export function createUsersScreen({ getClient }) {
  return createPollingScreen({
    getClient,
    intervalMs: REFRESH_INTERVAL_MS,
    load: getUsers,
    create: (host) => {
      const sessions = createTableScreen({
        title: TEXTS.sessionsTitle,
        emptyText: TEXTS.sessionsEmpty,
        compact: true,
        defaultSort: { key: 'user', direction: 'asc' },
        rowKey: (s) => `${s.userName}|${s.sessionName}|${s.sessionId}`,
        summary: (shown) => formatCount(shown.length, 'session', 'sessions'),
        columns: [
          { key: 'user', label: 'User', sortValue: (s) => s.userName.toLowerCase(), text: (s) => s.userName },
          { key: 'session', label: 'Session', width: '160px', sortValue: (s) => s.sessionName, text: (s) => s.sessionName || NO_VALUE },
          { key: 'id', label: 'ID', align: 'right', width: '80px', sortValue: (s) => s.sessionId ?? '', text: (s) => s.sessionId ?? NO_VALUE },
          { key: 'state', label: 'Status', width: '120px', sortValue: (s) => s.state, text: (s) => s.state },
          { key: 'logon', label: 'Logon time', width: '170px', sortValue: (s) => s.logonTime ?? '', text: (s) => formatDateTime(s.logonTime) },
          { key: 'host', label: 'Client', width: '160px', sortValue: (s) => s.host ?? '', text: (s) => s.host ?? NO_VALUE },
        ],
      });
      const usage = createTableScreen({
        title: TEXTS.usageTitle,
        emptyText: TEXTS.usageEmpty,
        compact: true,
        defaultSort: { key: 'cpu', direction: 'desc' },
        rowKey: (u) => u.userName,
        columns: [
          { key: 'user', label: 'User', sortValue: (u) => u.userName.toLowerCase(), text: (u) => u.userName },
          { key: 'processes', label: 'Processes', align: 'right', width: '110px', sortValue: (u) => u.processCount, text: (u) => formatInteger(u.processCount) },
          { key: 'cpu', label: 'CPU %', align: 'right', width: '100px', sortValue: (u) => u.cpuPercent, text: (u) => u.cpuPercent.toFixed(1) },
          { key: 'memory', label: 'Memory', align: 'right', width: '120px', sortValue: (u) => u.memoryBytes, text: (u) => formatBytes(u.memoryBytes) },
        ],
      });
      host.replaceChildren(h('div', { class: 'stack' }, [sessions.element, usage.element]));
      return {
        update(data) {
          sessions.update({ rows: data.sessions, unavailableReason: data.available ? null : data.reason });
          usage.update({ rows: data.users });
        },
      };
    },
  });
}
