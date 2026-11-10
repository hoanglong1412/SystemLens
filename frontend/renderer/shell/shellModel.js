/**
 * @file   shellModel.js
 * @brief  Turns the SystemOverview API object into the display data of the app shell
 *         (the object appShell.js renders). No DOM here.
 */
import {
  NO_VALUE, formatBitRate, formatDateTime, formatMegabytes, formatPercent, formatUptime,
} from '../ui/format.js';
import { SHELL_TEXTS } from './shellTexts.js';

/**
 * One quick-stat chip.
 * @param {string} id CSS modifier of the chip ("cpu", "mem", "disk").
 * @param {string} label
 * @param {number|null|undefined} percent
 * @returns {{id: string, label: string, value: string, percent: number}}
 */
function chip(id, label, percent) {
  return { id, label, value: formatPercent(percent ?? null), percent: percent ?? 0 };
}

/**
 * Tab captions, with the live counts where known ("Processes (253)").
 * @param {object|null} overview
 * @returns {string[]}
 */
function tabCaptions(overview) {
  return SHELL_TEXTS.tabs.map(({ name, count }) => {
    const value = count ? overview?.[count] : null;
    return typeof value === 'number' ? `${name} (${value})` : name;
  });
}

/**
 * Build the shell display data.
 * @param {object|null} overview SystemOverview from the API; null before the first response.
 * @param {boolean} connected Whether the last refresh succeeded.
 * @returns {object} Data for createAppShell / AppShell.update.
 */
export function toShellData(overview, connected) {
  const machine = overview?.machine;
  const texts = SHELL_TEXTS.status;
  return {
    windowTitle: SHELL_TEXTS.windowTitle,
    machine: {
      name: machine?.hostName || NO_VALUE,
      os: machine ? `${machine.osName} ${machine.architecture}`.trim() : NO_VALUE,
      details: machine
        ? [`Version ${machine.osVersion}`, `User: ${machine.userName}`, `Session: ${machine.sessionName}`]
        : [NO_VALUE],
    },
    quickStats: {
      uptime: `Up ${formatUptime(overview?.uptimeSeconds ?? null)}`,
      chips: [
        chip('cpu', 'CPU', overview?.cpuPercent),
        chip('mem', 'Mem', overview?.memoryPercent),
        chip('disk', 'Disk', overview?.diskPercent),
      ],
      network: {
        up: formatBitRate(overview?.network.sendBitsPerSecond ?? null),
        down: formatBitRate(overview?.network.receiveBitsPerSecond ?? null),
      },
      topProcess: {
        label: `Top: ${overview?.topMemoryProcess?.name ?? NO_VALUE}`,
        value: formatMegabytes(overview?.topMemoryProcess?.memoryBytes ?? null),
      },
    },
    tabs: tabCaptions(overview),
    activeTab: SHELL_TEXTS.activeTab,
    status: {
      connected,
      connection: [connected ? texts.connected : texts.disconnected, texts.updateSpeed],
      bootTime: `${texts.bootTimePrefix} ${formatDateTime(overview?.bootTime ?? null, { seconds: true })}`,
      action: texts.action,
    },
    tooltips: SHELL_TEXTS.tooltips,
  };
}
