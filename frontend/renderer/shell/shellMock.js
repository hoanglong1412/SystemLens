/**
 * @file   shellMock.js
 * @brief  [MOCK] Hard-coded values shown by the app shell (title bar, header, tabs, status bar),
 *         copied from the Figma design. To be replaced by backend data later.
 */
import { Placement } from '../ui/tooltip.js';

export const SHELL_MOCK = Object.freeze({
  windowTitle: 'SystemLens — Performance',
  machine: {
    name: 'DESKTOP-SYSLENS',
    os: 'Windows 10 Home x64',
    details: ['Version 10.0.19045', 'User: ASUS', 'Session: Console'],
  },
  quickStats: {
    uptime: 'Up 45:01:06:32',
    // percent drives the width of the coloured fill inside the chip.
    chips: [
      { id: 'cpu', label: 'CPU', value: '5%', percent: 5 },
      { id: 'mem', label: 'Mem', value: '87%', percent: 87 },
      { id: 'disk', label: 'Disk', value: '1%', percent: 1 },
    ],
    network: { up: '112 Kbps', down: '184 Kbps' },
    topProcess: { label: 'Top: msedge.exe', value: '612 MB' },
  },
  tabs: ['Processes (253)', 'Performance', 'App history', 'Startup', 'Users', 'Details', 'Services (281)'],
  activeTab: 'Performance',
  status: {
    connection: ['Connected to backend', 'Update speed: Normal (1 s)'],
    bootTime: 'System started at 14.09.2026 08:12:41',
    action: 'Desktop Monitor',
  },
  tooltips: {
    machine: {
      title: 'Machine info',
      body: 'Computer name, Windows edition/build and the signed-in user and session this snapshot comes from.',
    },
    quickStats: {
      title: 'Quick stats',
      body: 'Always-visible summary: uptime, total CPU / memory / disk load, network throughput and the process using the most memory.',
    },
    status: {
      title: 'Status',
      body: 'Connection to the SystemLens backend, how often the data refreshes, and when Windows last booted.',
      placement: Placement.TOP_START,
    },
  },
});
