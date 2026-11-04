/**
 * @file   shellModel.js
 * @brief  Turns backend data (SystemInfo, CpuMetrics) and the connection state into the view-model of
 *         the app shell (same shape as SHELL_MOCK). Memory, disk and network chips keep their mock
 *         values (out of scope: design D1).
 */
import {
  PLACEHOLDER, formatBytes, formatDateTime, formatPercent, formatUptime, isNumber,
} from '../ui/format.js';

/** Polling period shown in the status bar (must match app.js). */
const UPDATE_SPEED_TEXT = 'Update speed: Normal (1 s)';

/**
 * Build the shell view-model.
 * @param {object} input
 * @param {object|null} input.system SystemInfo, or null while unknown.
 * @param {object|null} input.metrics CpuMetrics, or null before the first response.
 * @param {boolean} input.connected Whether the last backend request succeeded.
 * @param {object} input.mock SHELL_MOCK (tabs, tooltips, chips that are not live yet).
 * @returns {object} SHELL_MOCK-shaped data, plus status.connected.
 */
export function buildShellModel({ system, metrics, connected, mock }) {
  const current = metrics?.current ?? null;
  const usage = current?.usagePercent;
  const top = current?.topProcesses?.[0] ?? null;

  const details = system
    ? [system.osVersion, `User: ${system.userName}`].filter((d) => d && d !== 'User: ')
    : [PLACEHOLDER];

  return {
    ...mock,
    machine: {
      name: system?.hostName || PLACEHOLDER,
      os: system?.osName || PLACEHOLDER,
      details,
    },
    quickStats: {
      ...mock.quickStats,
      uptime: `Up ${formatUptime(current?.uptimeSeconds)}`,
      chips: mock.quickStats.chips.map((chip) => (chip.id !== 'cpu' ? chip : {
        ...chip, value: formatPercent(usage), percent: isNumber(usage) ? usage : 0,
      })),
      topProcess: top
        ? { label: `Top: ${top.name}`, value: formatBytes(top.memoryBytes) }
        : { label: 'Top: —', value: '' },
    },
    status: {
      ...mock.status,
      connected,
      connection: [connected ? 'Connected to backend' : 'Backend unreachable', UPDATE_SPEED_TEXT],
      bootTime: `System started at ${formatDateTime(current?.bootTime, true)}`,
    },
  };
}
