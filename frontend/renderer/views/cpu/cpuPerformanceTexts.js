/**
 * @file   cpuPerformanceTexts.js
 * @brief  Static texts of the Performance > CPU screen: card titles, column names and tooltips.
 */
import { Placement } from '../../ui/tooltip.js';

export const CPU_TEXTS = Object.freeze({
  title: 'CPU',
  subTabs: ['Overview', 'Per-core'],
  activeSubTab: 'Overview',
  cores: { coreLabel: 'Core', title: 'Logical processors' },
  gauges: { title: 'Live gauges', usage: 'Usage', clock: 'Clock', temperature: 'Temp' },
  utilization: {
    title: '% Utilization',
    max: '100%',
    xStart: '60 seconds',
    xEnd: '0',
    usage: 'Usage',
    clock: 'Clock',
    temperature: 'Temperature',
  },
  topProcesses: { title: 'Top CPU processes', columns: ['PID', 'Name', 'CPU %', 'Memory'] },
  summary: {
    utilization: 'Utilization',
    speed: 'Speed',
    temperature: 'Temperature',
    processes: 'Processes',
    threads: 'Threads',
    handles: 'Handles',
    uptime: 'Up time',
    started: 'Started',
  },
  specifications: {
    title: 'Specifications',
    baseSpeed: 'Base speed',
    sockets: 'Sockets',
    cores: 'Cores',
    logicalProcessors: 'Logical processors',
    virtualization: 'Virtualization',
    virtualizationStates: { enabled: 'Enabled', disabled: 'Disabled' },
    l1Cache: 'L1 cache',
    l2Cache: 'L2 cache',
    l3Cache: 'L3 cache',
  },
  breakdown: {
    title: 'CPU time breakdown',
    scope: ['Last 1 s', 'all cores'],
    user: 'User',
    kernel: 'Kernel',
    dpc: 'DPC',
    interrupt: 'Interrupt',
    idle: 'Idle',
    trendCaption: ['User vs Kernel', '60 s'],
  },
  scheduler: {
    title: 'Scheduler activity',
    unit: 'per second',
    contextSwitches: 'Context switches',
    interrupts: 'Interrupts',
    systemCalls: 'System calls',
    dpcs: 'DPCs queued',
    sourcePrefix: 'Source:',
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
      body: 'The 10 processes that used the most CPU in the last interval. A brighter green cell means a heavier load.',
    },
    summary: {
      title: 'Summary',
      body: 'Headline CPU numbers: usage, speed, temperature, process / thread / handle counts and the time since the system booted.',
      placement: Placement.BOTTOM_END,
    },
    specifications: {
      title: 'Specifications',
      body: 'Static facts about the processor: base clock, sockets, physical cores, logical processors, virtualization and cache sizes.',
      placement: Placement.BOTTOM_END,
    },
    breakdown: {
      title: 'CPU time breakdown',
      body: 'How CPU time was split: User = applications, Kernel = the operating system itself, DPC / Interrupt = driver and hardware work (softirq / irq on Linux), Idle = nothing to run.',
    },
    scheduler: {
      title: 'Scheduler activity',
      body: 'How busy the scheduler is: thread context switches, hardware interrupts, system calls and deferred procedure calls (softirqs on Linux) per second.',
      placement: Placement.BOTTOM_END,
    },
  },
});
