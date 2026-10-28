/**
 * @file   cpuPerformanceMock.js
 * @brief  [MOCK] Hard-coded data of the Performance > CPU screen, copied from the Figma design
 *         (series were digitized from the design image). To be replaced by backend data later.
 *
 * Series are percentages (0..100 of the plot height), oldest sample first.
 */
import { Placement } from '../../ui/tooltip.js';

export const CPU_PERFORMANCE_MOCK = Object.freeze({
  resources: [
    {
      id: 'cpu', name: 'CPU', detail: ['5%', '2.61 GHz'], color: 'var(--green)', fill: true, selected: true,
      series: [12.5, 16.5, 12.5, 8.5, 10.5, 14.5, 12.5, 12.5, 10.5, 12.5, 14.5, 14.5, 14.5, 12.5, 12.5, 12.5, 14.5, 14.5, 12.5, 8.5, 10.5, 14.5, 14.5, 12.5, 10.5, 10.5, 12.5, 12.5, 14.5, 16.5, 14.5, 12.5, 10.5, 12.5, 16.5, 16.5, 14.5, 12.5, 12.5, 12.5, 16.5, 26.5, 44.5, 68.5, 60.5, 30.5, 20.5, 12.5, 10.5, 8.5, 8.5, 10.5, 10.5, 10.5, 14.5, 16.5, 14.5, 14.5, 16.5, 18.5, 20.5, 22.5, 24.5, 26.5, 28.5, 30.5, 32.5, 34.5, 36.5, 38.5, 38.5],
    },
    {
      id: 'memory', name: 'Memory', detail: ['6.9/7.9 GB (87%)'], color: 'var(--blue)', fill: true,
      series: [88.5, 88.5, 86.5, 84.5, 84.5, 85.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 88.5, 86.5, 84.5, 86.5, 86.5, 86.5, 86.5, 86.5, 88.5, 88.5, 86.5, 86.5, 84.5, 84.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 88.5, 86.5, 86.5, 88.5, 87.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 88.5, 88.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5, 86.5],
    },
    {
      id: 'disk0', name: 'Disk 0 (C: E:)', detail: ['SSD', '1%'], color: 'var(--lime)', fill: true,
      series: [4.5, 4.5, 2.5, 2.5, 2.5, 0.5, 2.5, 2.5, 0.5, 0.5, 2.5, 4.5, 2.5, 2.5, 16.5, 36.5, 58.5, 50.5, 24.5, 8.5, 2.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 2.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 6.5, 20.5, 38.5, 44.5, 26.5, 12.5, 4.5, 2.5, 2.5, 0.5, 2.5, 4.5, 6.5, 6.5, 8.5, 10.5, 12.5, 14.5, 16.5, 16.5, 18.5, 20.5, 22.5, 24.5, 24.5, 26.5, 28.5, 28.5],
    },
    {
      id: 'ethernet', name: 'Ethernet', detail: ['vEthernet (WSL)', '0 Kbps'], color: 'var(--orange-dim)', fill: true,
      series: new Array(71).fill(0.5),
    },
    {
      id: 'wifi', name: 'Wi-Fi', detail: ['S 112  R 184 Kbps'], color: 'var(--orange-dark)', fill: true,
      series: [28.5, 24.5, 26.5, 28.5, 24.5, 20.5, 18.5, 20.5, 28.5, 34.5, 36.5, 56.5, 78.5, 66.5, 36.5, 32.5, 30.5, 22.5, 26.5, 26.5, 20.5, 22.5, 24.5, 24.5, 28.5, 30.5, 32.5, 32.5, 30.5, 36.5, 60.5, 88.5, 78.5, 44.5, 32.5, 26.5, 24.5, 24.5, 26.5, 24.5, 28.5, 30.5, 26.5, 26.5, 30.5, 22.5, 18.5, 20.5, 24.5, 24.5, 22.5, 24.5, 28.5, 30.5, 48.5, 68.5, 58.5, 30.5, 24.5, 20.5, 20.5, 28.5, 36.5, 44.5, 52.5, 60.5, 68.5, 76.5, 84.5, 92.5, 92.5],
    },
    {
      id: 'gpu0', name: 'GPU 0', detail: ['Intel UHD 630', '0%'], color: 'var(--purple)', fill: true,
      series: [2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 0.5, 0.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 2.5, 2.5, 2.5, 2.5, 0.5, 0.5, 0.5, 0.5, 0.5, 2.5, 0.5, 0.5, 0.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 2.5, 2.5, 2.5, 2.5, 0.5, 0.5, 2.5, 2.5, 2.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5],
    },
    {
      id: 'gpu1', name: 'GPU 1', detail: ['GTX 1050', '0% (53 °C)'], color: 'var(--purple)', fill: true,
      series: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 2.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.9, 1.3, 1.7, 2.1, 2.5, 0.5, 0.5, 0.5, 0.5, 0.5, 1.5, 2.5, 2.5],
    },
  ],

  header: {
    title: 'CPU',
    model: 'Intel(R) Core(TM) i5-8300H CPU @ 2.30GHz',
    topology: ['1 socket', '4 cores', '8 logical processors'],
    subTabs: ['Overview', 'Per-core'],
    activeSubTab: 'Overview',
  },

  gauges: [
    { label: 'Usage', value: '5%', percent: 5, color: 'var(--green)' },
    { label: 'Clock', value: '2.61 GHz', percent: 65, color: 'var(--orange)' },
    { label: 'Temp', value: '62 °C', percent: 62, color: 'var(--yellow)' },
  ],

  utilization: {
    title: '% Utilization',
    max: '100%',
    xStart: '60 seconds',
    xEnd: '0',
    series: [
      {
        name: 'Usage', color: 'var(--green)', fill: true,
        values: [3.2, 5.4, 4.5, 3.2, 8, 5.4, 5.8, 5.8, 6.3, 15.5, 31.7, 12.4, 4.1, 5.2, 6.5, 7.6, 4.9, 3.6, 4.1, 7.1, 6.3, 6.3, 5.8, 4.5, 4.9, 7.6, 3.2, 4.5, 6.3, 7.1, 3.2, 5.4, 8, 5.8, 6.3, 47.5, 94, 38, 8.9, 3.2, 7.1, 7.8, 8.4, 4.5, 4.1, 7.6, 8.4, 7.6, 8.9, 7.6, 6.3, 19, 36.5, 15.5, 8.4, 9.8, 35.6, 69.4, 29.9, 7.1, 8.4],
      },
      {
        name: 'Clock', color: 'var(--orange)',
        values: [58, 58.9, 58, 60.2, 57.1, 61.1, 61.1, 57.3, 59.8, 57.6, 55.4, 54.1, 54.9, 60.2, 54.9, 56.7, 58, 56.7, 57.6, 54.5, 56.7, 55.4, 58, 58, 61.1, 54.9, 61.5, 61.5, 59.8, 58, 56.7, 60.2, 58.7, 57.6, 56.3, 43.1, 84.8, 37.4, 58, 60.6, 54.9, 55.4, 61.1, 59.1, 56.3, 59.3, 56.7, 60.3, 57.6, 57.6, 55.6, 54.9, 59.8, 60.6, 56.3, 59.3, 40.5, 76.9, 34.3, 53.6, 54.1],
      },
      {
        name: 'Temperature', color: 'var(--yellow)',
        values: [58.9, 59.8, 59.5, 60.6, 59.3, 60.2, 61.1, 59.9, 60.6, 59.3, 60.2, 59.8, 59.3, 59.3, 59.8, 60.2, 59.8, 59.3, 59.8, 59.8, 58.9, 60.2, 60.6, 59.8, 59.9, 59.3, 60.6, 59.5, 58.9, 60.2, 59.8, 59.3, 60.1, 61.1, 59.8, 58.4, 37.4, 71.2, 32.1, 59.3, 59.3, 59.3, 59.5, 61.1, 60.3, 60.2, 59.8, 59.8, 59.8, 60.2, 58.9, 60.6, 60.2, 58.9, 60.6, 59.8, 60.2, 58.9, 60.6, 61.1, 61.1],
      },
    ],
  },

  topProcesses: {
    title: 'Top CPU processes (10)',
    columns: ['PID', 'Name', 'CPU %', 'Memory'],
    // cpuShade: background of the CPU % cell, darker = lighter load (precomputed from the design).
    rows: [
      { pid: 15068, name: 'SystemLens.exe', cpu: '2.61', memory: '111.1 MB', cpuShade: '#2daf6a', self: true },
      { pid: 2284, name: 'msedge.exe', cpu: '1.29', memory: '612.4 MB', cpuShade: '#29764f' },
      { pid: 2140, name: 'Code.exe', cpu: '0.87', memory: '467.5 MB', cpuShade: '#276547' },
      { pid: 2552, name: 'Desktop Window...', cpu: '0.53', memory: '455.2 MB', cpuShade: '#265540' },
      { pid: 4, name: 'System', cpu: '0.19', memory: '0.1 MB', cpuShade: '#254739' },
      { pid: 17044, name: 'explorer.exe', cpu: '0.14', memory: '193.0 MB', cpuShade: '#254438' },
      { pid: 2956, name: 'WmiPrvSE.exe', cpu: '0.09', memory: '19.3 MB', cpuShade: '#254237' },
      { pid: 18540, name: 'svchost.exe', cpu: '0.08', memory: '12.6 MB', cpuShade: '#254237' },
      { pid: 5756, name: 'RuntimeBroker.exe', cpu: '0.07', memory: '8.2 MB', cpuShade: '#254237' },
      { pid: 22096, name: 'wsl.exe', cpu: '0.06', memory: '54.4 MB', cpuShade: '#254036' },
    ],
  },

  summary: {
    metrics: [
      { label: 'Utilization', value: '5%' },
      { label: 'Speed', value: '2.61 GHz' },
      { label: 'Temperature', value: '62 °C' },
      { label: 'Processes', value: '253' },
      { label: 'Threads', value: '3,278' },
      { label: 'Handles', value: '114,453' },
    ],
    uptime: { label: 'Up time', value: '45:01:06:32', startedLabel: 'Started', started: '14.09.2026 08:12' },
  },

  specifications: {
    title: 'Specifications',
    columns: [
      [['Base speed', '2.30 GHz'], ['Sockets', '1'], ['Cores', '4'], ['Logical processors', '8']],
      [['Virtualization', 'Enabled'], ['L1 cache', '256 KB'], ['L2 cache', '1.0 MB'], ['L3 cache', '8.0 MB']],
    ],
  },

  breakdown: {
    title: 'CPU time breakdown',
    scope: ['Last 1 s', 'all cores'],
    // width: segment width in px on the 584px bar, as drawn in the design.
    parts: [
      { name: 'User', percent: '3.1%', color: 'var(--green)', width: 18 },
      { name: 'Kernel', percent: '1.6%', color: 'var(--orange)', width: 9 },
      { name: 'DPC', percent: '0.2%', color: 'var(--purple)', width: 6 },
      { name: 'Interrupt', percent: '0.1%', color: 'var(--yellow)', width: 6 },
      { name: 'Idle', percent: '95.0%', color: 'var(--idle)', width: 545 },
    ],
    trendCaption: ['User vs Kernel', '60 s'],
    trend: [
      {
        name: 'User', color: 'var(--green)', fill: true,
        values: [4.3, 4.3, 4.3, 4.3, 4.3, 6.3, 5.6, 4.8, 4.3, 6.3, 5.3, 4.3, 4.3, 4.3, 4.3, 6.3, 4.3, 6.3, 4.3, 6.3, 5.2, 4.3, 6.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 6.3, 6.3, 6.3, 4.8, 4.3, 6.3, 15.9, 98.6, 10.5, 4.3, 4.6, 5.2, 5.9, 6.3, 6.3, 6.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 4.3, 6.3, 10.1, 65.9, 13.9, 13.9, 13.9],
      },
      {
        name: 'Kernel', color: 'var(--orange)',
        values: [0.5, 2.4, 0.5, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 0.8, 0.5, 1.2, 2.4, 0.5, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 2, 1, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 0.5, 2.4, 6.3, 38.9, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 2.4, 0.5, 2.4, 2.4, 2.4, 2.4, 2.4, 0.5, 1.4, 2.4, 2.4, 2.4, 2.4, 25.5, 2.4, 2.4, 2.4],
      },
    ],
  },

  scheduler: {
    title: 'Scheduler activity',
    unit: 'per second',
    metrics: [
      { label: 'Context switches', value: '24,318' },
      { label: 'Interrupts', value: '9,842' },
      { label: 'System calls', value: '61,207' },
      { label: 'DPCs queued', value: '1,135' },
    ],
    source: 'Source: PDH \\System, \\Processor(_Total)',
  },

  tooltips: {
    resources: {
      title: 'Resources',
      body: 'Every monitored resource with a 60-second sparkline. Select one to open its detail view on the right.',
    },
    gauges: {
      title: 'Live gauges',
      body: 'Current CPU usage, clock speed and package temperature, drawn as LED bars for an at-a-glance reading.',
    },
    utilization: {
      title: '% Utilization',
      body: 'Total CPU usage (green area), current clock (orange) and temperature (yellow) over the last 60 seconds, sampled every second.',
    },
    topProcesses: {
      title: 'Top CPU processes',
      body: 'The 10 processes that used the most CPU in the last interval. A darker green cell means a heavier load.',
    },
    summary: {
      title: 'Summary',
      body: 'Headline CPU numbers: usage, speed, temperature, process / thread / handle counts and the time since Windows booted.',
      placement: Placement.BOTTOM_END,
    },
    specifications: {
      title: 'Specifications',
      body: 'Static facts about the processor: base clock, sockets, physical cores, logical processors, virtualization and cache sizes.',
      placement: Placement.BOTTOM_END,
    },
    breakdown: {
      title: 'CPU time breakdown',
      body: 'How CPU time was split: User = applications, Kernel = Windows itself, DPC / Interrupt = driver and hardware work, Idle = nothing to run.',
    },
    scheduler: {
      title: 'Scheduler activity',
      body: 'How busy the Windows scheduler is: thread context switches, hardware interrupts, system calls and deferred procedure calls per second.',
      placement: Placement.BOTTOM_END,
    },
  },
});
