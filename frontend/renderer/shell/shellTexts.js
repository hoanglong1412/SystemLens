/**
 * @file   shellTexts.js
 * @brief  Static texts of the app shell: window title, tab names, status bar wording and tooltips.
 */
import { Placement } from '../ui/tooltip.js';

export const SHELL_TEXTS = Object.freeze({
  /** Window title prefix; the active tab name follows. */
  windowTitle: 'SystemLens',
  /** Tabs in display order. count: key of the live count shown in parentheses, when there is one. */
  tabs: [
    { id: 'processes', name: 'Processes', count: 'processCount' },
    { id: 'performance', name: 'Performance' },
    { id: 'history', name: 'App history' },
    { id: 'startup', name: 'Startup' },
    { id: 'users', name: 'Users' },
    { id: 'details', name: 'Details' },
    { id: 'services', name: 'Services', count: 'serviceCount' },
  ],
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
