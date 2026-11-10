/**
 * @file   shellTexts.js
 * @brief  Static texts of the app shell: window title, tab names, status bar wording and tooltips.
 */
import { Placement } from '../ui/tooltip.js';

export const SHELL_TEXTS = Object.freeze({
  windowTitle: 'SystemLens — Performance',
  /** Tabs in display order. count: key of the live count shown in parentheses, when there is one. */
  tabs: [
    { name: 'Processes', count: 'processCount' },
    { name: 'Performance' },
    { name: 'App history' },
    { name: 'Startup' },
    { name: 'Users' },
    { name: 'Details' },
    { name: 'Services', count: 'serviceCount' },
  ],
  activeTab: 'Performance',
  status: {
    connected: 'Connected to backend',
    disconnected: 'Disconnected from backend',
    updateSpeed: 'Update speed: Normal (1 s)',
    bootTimePrefix: 'System started at',
    action: 'Desktop Monitor',
  },
  tooltips: {
    machine: {
      title: 'Machine info',
      body: 'Computer name, operating system edition and version, and the signed-in user and session this snapshot comes from.',
    },
    quickStats: {
      title: 'Quick stats',
      body: 'Always-visible summary: uptime, total CPU / memory / disk load, network throughput and the process using the most memory.',
    },
    status: {
      title: 'Status',
      body: 'Connection to the SystemLens backend, how often the data refreshes, and when the system last booted.',
      placement: Placement.TOP_START,
    },
  },
});
