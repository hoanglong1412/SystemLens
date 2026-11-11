/**
 * @file   cpuPerformanceModel.js
 * @brief  Turns the CpuPerformance and CpuCores API objects into the display data of the Performance > CPU
 *         panel (the object cpuPerformanceView.js renders). No DOM here.
 *
 * Series are percentages (0..100 of the plot height), oldest sample first.
 */
import {
  NO_VALUE, formatCacheSize, formatCount, formatDateTime, formatFrequency,
  formatGigabytesValue, formatInteger, formatMegabytes, formatPercent, formatTemperature, formatUptime,
} from '../../ui/format.js';
import { MAX_PERCENT, gaplessHistory, padHistory } from '../../ui/series.js';
import { CPU_TEXTS } from './cpuPerformanceTexts.js';

/** Longest process name that fits its column; longer names end with "...". */
const MAX_PROCESS_NAME_LENGTH = 17;
const ELLIPSIS = '...';
/** Width of the CPU time breakdown bar and the smallest visible segment (px). */
const BREAKDOWN_BAR_WIDTH = 584;
const BREAKDOWN_MIN_SEGMENT_WIDTH = 6;
/** Background of the CPU % cell: from an idle row to the busiest row of the table (RGB). */
const CPU_SHADE_LOW = [0x25, 0x40, 0x36];
const CPU_SHADE_HIGH = [0x2d, 0xaf, 0x6a];

/**
 * Shorten a process name to the width of its column.
 * @param {string} name
 * @returns {string}
 */
function shortProcessName(name) {
  return name.length > MAX_PROCESS_NAME_LENGTH
    ? name.slice(0, MAX_PROCESS_NAME_LENGTH - ELLIPSIS.length) + ELLIPSIS
    : name;
}

/**
 * Background colour of a CPU % cell, interpolated between the idle and the busiest shade.
 * @param {number} cpuPercent CPU usage of the row.
 * @param {number} maxCpuPercent Highest CPU usage in the table.
 * @returns {string} CSS rgb() colour.
 */
function cpuShade(cpuPercent, maxCpuPercent) {
  const ratio = maxCpuPercent > 0 ? Math.min(cpuPercent / maxCpuPercent, 1) : 0;
  const channels = CPU_SHADE_LOW.map((low, i) => Math.round(low + (CPU_SHADE_HIGH[i] - low) * ratio));
  return `rgb(${channels.join(', ')})`;
}

/**
 * Rows of the top processes table.
 * @param {object[]} processes CpuProcess[], highest CPU first.
 * @returns {object[]}
 */
function toProcessRows(processes) {
  const maxCpuPercent = Math.max(0, ...processes.map((process) => process.cpuPercent));
  return processes.map((process) => ({
    pid: process.pid,
    name: shortProcessName(process.name),
    cpu: process.cpuPercent.toFixed(2),
    memory: formatMegabytes(process.memoryBytes, 1),
    cpuShade: cpuShade(process.cpuPercent, maxCpuPercent),
    self: process.isSelf,
  }));
}

/**
 * Segments of the CPU time breakdown bar and legend.
 * @param {object|undefined} breakdown CpuTimeBreakdown.
 * @returns {object[]} One part per category; width null = takes the rest of the bar (Idle).
 */
function toBreakdownParts(breakdown) {
  const texts = CPU_TEXTS.breakdown;
  // A share too small for one pixel would vanish, so every non-zero part gets a minimum width.
  const width = (percent) => (percent > 0
    ? Math.max(BREAKDOWN_MIN_SEGMENT_WIDTH, Math.round((percent / MAX_PERCENT) * BREAKDOWN_BAR_WIDTH))
    : 0);
  const part = (name, percent, color, isRest = false) => ({
    name,
    percent: formatPercent(percent ?? null, 1),
    color,
    width: isRest ? null : width(percent ?? 0),
  });
  return [
    part(texts.user, breakdown?.userPercent, 'var(--green)'),
    part(texts.kernel, breakdown?.kernelPercent, 'var(--orange)'),
    part(texts.dpc, breakdown?.dpcPercent, 'var(--purple)'),
    part(texts.interrupt, breakdown?.interruptPercent, 'var(--yellow)'),
    part(texts.idle, breakdown?.idlePercent, 'var(--idle)', true),
  ];
}

/**
 * One card per logical processor.
 * @param {object|null} cores CpuCores from the API; null before the first response.
 * @returns {Array<{label: string, value: string, series: number[]}>}
 */
function toCores(cores) {
  return (cores?.cores ?? []).map((core) => ({
    label: `${CPU_TEXTS.cores.coreLabel} ${core.index}`,
    value: formatPercent(core.usagePercent),
    series: padHistory(core.history, 0),
  }));
}

/**
 * Build the display data of the CPU panel.
 * @param {object|null} cpu CpuPerformance from the API; null before the first response.
 * @param {object|null} cores CpuCores from the API; null before the first response.
 * @returns {object} Data for the CPU panel's update().
 */
export function toCpuViewData(cpu, cores) {
  const texts = CPU_TEXTS;
  const info = cpu?.info;
  const scheduler = cpu?.scheduler;
  const usage = formatPercent(cpu?.usagePercent ?? null);
  const frequency = formatFrequency(cpu?.frequencyMhz ?? null);
  const temperature = formatTemperature(cpu?.temperatureCelsius ?? null);

  const utilizationSeries = [
    { name: texts.utilization.usage, color: 'var(--green)', fill: true, values: padHistory(cpu?.history.usagePercent, 0) },
    { name: texts.utilization.clock, color: 'var(--orange)', values: gaplessHistory(cpu?.history.frequencyPercent) },
    { name: texts.utilization.temperature, color: 'var(--yellow)', values: gaplessHistory(cpu?.history.temperatureCelsius) },
  ];

  return {
    cores: toCores(cores),

    header: {
      model: info?.model ?? NO_VALUE,
      topology: info
        ? [
          formatCount(info.sockets, 'socket', 'sockets'),
          formatCount(info.cores, 'core', 'cores'),
          formatCount(info.logicalProcessors, 'logical processor', 'logical processors'),
        ]
        : [NO_VALUE],
    },

    gauges: [
      { label: texts.gauges.usage, value: usage, percent: cpu?.usagePercent ?? 0, color: 'var(--green)' },
      { label: texts.gauges.clock, value: frequency, percent: cpu?.frequencyPercent ?? 0, color: 'var(--orange)' },
      {
        label: texts.gauges.temperature,
        value: temperature,
        // The gauge shows degrees Celsius on a 0..100 scale.
        percent: Math.min(cpu?.temperatureCelsius ?? 0, MAX_PERCENT),
        color: 'var(--yellow)',
      },
    ],

    utilization: {
      legend: utilizationSeries.map(({ name, color }) => ({ name, color })),
      // Series without any sample (e.g. no temperature sensor) are not drawn.
      series: utilizationSeries.filter((series) => series.values !== null),
    },

    topProcesses: {
      title: `${texts.topProcesses.title} (${cpu?.topProcesses.length ?? 0})`,
      rows: toProcessRows(cpu?.topProcesses ?? []),
    },

    summary: {
      metrics: [
        { label: texts.summary.utilization, value: usage },
        { label: texts.summary.speed, value: frequency },
        { label: texts.summary.temperature, value: temperature },
        { label: texts.summary.processes, value: formatInteger(cpu?.processCount ?? null) },
        { label: texts.summary.threads, value: formatInteger(cpu?.threadCount ?? null) },
        { label: texts.summary.handles, value: formatInteger(cpu?.handleCount ?? null) },
      ],
      uptime: {
        label: texts.summary.uptime,
        value: formatUptime(cpu?.uptimeSeconds ?? null),
        startedLabel: texts.summary.started,
        started: formatDateTime(cpu?.bootTime ?? null),
      },
    },

    specifications: {
      columns: [
        [
          [texts.specifications.baseSpeed, formatFrequency(info?.baseSpeedMhz ?? null)],
          [texts.specifications.sockets, formatInteger(info?.sockets ?? null)],
          [texts.specifications.cores, formatInteger(info?.cores ?? null)],
          [texts.specifications.logicalProcessors, formatInteger(info?.logicalProcessors ?? null)],
        ],
        [
          [texts.specifications.virtualization,
            texts.specifications.virtualizationStates[info?.virtualization] ?? NO_VALUE],
          [texts.specifications.l1Cache, formatCacheSize(info?.l1CacheBytes ?? null)],
          [texts.specifications.l2Cache, formatCacheSize(info?.l2CacheBytes ?? null)],
          [texts.specifications.l3Cache, formatCacheSize(info?.l3CacheBytes ?? null)],
        ],
      ],
    },

    breakdown: {
      parts: toBreakdownParts(cpu?.timeBreakdown),
      trend: [
        { color: 'var(--green)', fill: true, values: padHistory(cpu?.timeBreakdown.history.userPercent, 0) },
        { color: 'var(--orange)', values: padHistory(cpu?.timeBreakdown.history.kernelPercent, 0) },
      ],
    },

    scheduler: {
      metrics: [
        { label: texts.scheduler.contextSwitches, value: formatInteger(scheduler?.contextSwitchesPerSecond ?? null) },
        { label: texts.scheduler.interrupts, value: formatInteger(scheduler?.interruptsPerSecond ?? null) },
        { label: texts.scheduler.systemCalls, value: formatInteger(scheduler?.systemCallsPerSecond ?? null) },
        { label: texts.scheduler.dpcs, value: formatInteger(scheduler?.dpcsPerSecond ?? null) },
      ],
      source: `${texts.scheduler.sourcePrefix} ${scheduler?.source ?? NO_VALUE}`,
    },
  };
}
