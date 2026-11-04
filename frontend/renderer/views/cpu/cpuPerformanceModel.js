/**
 * @file   cpuPerformanceModel.js
 * @brief  Turns backend data (CpuInfo + CpuMetrics, api/openapi.yaml) into the view-model rendered by
 *         cpuPerformanceView.js (same shape as CPU_PERFORMANCE_MOCK). All formatting and scaling of the
 *         CPU screen lives here, so the view only lays data out.
 *
 * Sidebar entries other than CPU, and the tooltips, come from the mock (out of scope: design D1).
 */
import {
  PLACEHOLDER, formatBytes, formatCelsius, formatDateTime, formatGhz, formatInteger, formatPercent, formatUptime,
  isNumber,
} from '../../ui/format.js';

/** Samples across the charts (60 s at 1 s, matches CpuMetrics.historyLength). */
export const HISTORY_CAPACITY = 60;
/** Temperature full scale of the gauge and the chart. */
const TEMPERATURE_FULL_SCALE = 100;
/** Clock full scale when maxSpeedMhz is unknown: twice the base clock (design D6). */
const BASE_SPEED_SCALE_FACTOR = 2;
/** Width of the CPU time breakdown bar (px, cpuPerformance.css). */
const BREAKDOWN_BAR_WIDTH = 584;
/** Narrowest visible segment for a non-zero part (px). */
const MIN_SEGMENT_WIDTH = 2;
/** Background of the CPU % cell: lightest load → heaviest load (from the design). */
const SHADE_LOW = [0x25, 0x40, 0x36];
const SHADE_HIGH = [0x2d, 0xaf, 0x6a];
/** A process whose name starts with this is SystemLens itself (highlighted like in the design). */
const SELF_NAME_PREFIX = 'systemlens';

/**
 * Replace null samples by the nearest earlier value (or the first known one) so lines stay continuous.
 * @param {Array<number|null>} values
 * @returns {number[]} Empty when every sample is null.
 */
function fillGaps(values) {
  const firstKnown = values.find(isNumber);
  if (firstKnown === undefined) return [];
  let last = firstKnown;
  return values.map((v) => {
    if (isNumber(v)) last = v;
    return last;
  });
}

/**
 * @param {number} value
 * @param {number} scale Full scale.
 * @returns {number} value as 0..100 of scale.
 */
function toPercentOf(value, scale) {
  return Math.min(Math.max((value / scale) * 100, 0), 100);
}

/**
 * Full scale of the clock gauge / series.
 * @param {object|null} info CpuInfo.
 * @param {number|null} currentMhz Current clock, used when the info has no speed at all.
 * @returns {number|null} MHz, or null when nothing is known.
 */
export function clockScaleMhz(info, currentMhz) {
  if (info && isNumber(info.maxSpeedMhz)) return info.maxSpeedMhz;
  if (info && isNumber(info.baseSpeedMhz)) return info.baseSpeedMhz * BASE_SPEED_SCALE_FACTOR;
  return isNumber(currentMhz) ? currentMhz * BASE_SPEED_SCALE_FACTOR : null;
}

/**
 * Background colour of a CPU % cell, interpolated between the design's lightest and darkest shade.
 * @param {number} cpuPercent Process CPU %.
 * @param {number} maxPercent Highest CPU % in the table.
 * @returns {string} "#rrggbb".
 */
export function cpuShade(cpuPercent, maxPercent) {
  const t = maxPercent > 0 ? Math.min(Math.max(cpuPercent / maxPercent, 0), 1) : 0;
  const channel = (i) => Math.round(SHADE_LOW[i] + (SHADE_HIGH[i] - SHADE_LOW[i]) * t);
  return `#${[0, 1, 2].map((i) => channel(i).toString(16).padStart(2, '0')).join('')}`;
}

/**
 * Segment widths of the breakdown bar: every non-zero part stays visible, idle takes the rest.
 * @param {number[]} percents Non-idle parts (0..100).
 * @returns {{parts: number[], idle: number}} Widths in px.
 */
export function breakdownWidths(percents) {
  const parts = percents.map((p) => (isNumber(p) && p > 0
    ? Math.max(MIN_SEGMENT_WIDTH, Math.round((p / 100) * BREAKDOWN_BAR_WIDTH)) : 0));
  const used = parts.reduce((a, b) => a + b, 0);
  return { parts, idle: Math.max(BREAKDOWN_BAR_WIDTH - used, 0) };
}

/**
 * @param {number} count
 * @param {string} word Singular noun.
 * @returns {string} "1 socket", "2 sockets".
 */
function plural(count, word) {
  return `${count} ${word}${count === 1 ? '' : 's'}`;
}

/**
 * Specification rows.
 * @param {object|null} info CpuInfo.
 * @returns {Array<Array<[string, string]>>} Two columns.
 */
function specifications(info) {
  const value = (v, fmt = String) => (v === null || v === undefined ? PLACEHOLDER : fmt(v));
  const virtualization = { enabled: 'Enabled', disabled: 'Disabled', unknown: 'Unknown' };
  return [
    [
      ['Base speed', value(info?.baseSpeedMhz, formatGhz)],
      ['Sockets', value(info?.sockets)],
      ['Cores', value(info?.cores)],
      ['Logical processors', value(info?.logicalProcessors)],
    ],
    [
      ['Virtualization', info ? virtualization[info.virtualization] ?? PLACEHOLDER : PLACEHOLDER],
      ['L1 cache', value(info?.cache?.l1Bytes, formatBytes)],
      ['L2 cache', value(info?.cache?.l2Bytes, formatBytes)],
      ['L3 cache', value(info?.cache?.l3Bytes, formatBytes)],
    ],
  ];
}

/**
 * Build the CPU screen view-model.
 * @param {object} input
 * @param {object|null} input.info CpuInfo, or null while unknown.
 * @param {object|null} input.metrics CpuMetrics, or null before the first response.
 * @param {number[][]} [input.perCoreHistory] Usage history per logical processor (oldest first).
 * @param {object} input.mock CPU_PERFORMANCE_MOCK (sidebar entries other than CPU, tooltips).
 * @returns {object} View-model shaped like CPU_PERFORMANCE_MOCK, plus perCore.
 */
export function buildCpuViewModel({ info, metrics, perCoreHistory = [], mock }) {
  const current = metrics?.current ?? null;
  const history = metrics?.history ?? {
    usagePercent: [], speedMhz: [], temperatureCelsius: [], userPercent: [], kernelPercent: [],
  };
  const usage = current?.usagePercent ?? null;
  const speed = current?.speedMhz ?? null;
  const temperature = current?.temperatureCelsius ?? null;
  const scale = clockScaleMhz(info, speed);

  const resources = mock.resources.map((res) => (res.id !== 'cpu' ? { ...res, selected: false } : {
    ...res,
    selected: true,
    detail: [formatPercent(usage), formatGhz(speed)],
    series: history.usagePercent,
  }));

  const clockSeries = scale ? fillGaps(history.speedMhz).map((mhz) => toPercentOf(mhz, scale)) : [];
  const temperatureSeries = fillGaps(history.temperatureCelsius).map((c) => toPercentOf(c, TEMPERATURE_FULL_SCALE));

  const rows = current?.topProcesses ?? [];
  const maxCpu = rows.reduce((m, r) => Math.max(m, r.cpuPercent), 0);

  const b = current?.breakdown ?? null;
  const nonIdle = b ? [b.userPercent, b.kernelPercent, b.dpcPercent, b.interruptPercent] : [0, 0, 0, 0];
  const widths = breakdownWidths(nonIdle);
  const partPercent = (p) => (isNumber(p) ? `${p.toFixed(1)}%` : PLACEHOLDER);
  const sc = current?.scheduler ?? {};

  return {
    resources,
    header: {
      title: 'CPU',
      model: info?.model ?? PLACEHOLDER,
      topology: info
        ? [plural(info.sockets, 'socket'), plural(info.cores, 'core'), plural(info.logicalProcessors, 'logical processor')]
        : [PLACEHOLDER],
      subTabs: mock.header.subTabs,
    },
    gauges: [
      { label: 'Usage', value: formatPercent(usage), percent: isNumber(usage) ? usage : 0, color: 'var(--green)' },
      {
        label: 'Clock', value: formatGhz(speed), percent: isNumber(speed) && scale ? toPercentOf(speed, scale) : 0,
        color: 'var(--orange)',
      },
      {
        label: 'Temp', value: formatCelsius(temperature),
        percent: isNumber(temperature) ? toPercentOf(temperature, TEMPERATURE_FULL_SCALE) : 0, color: 'var(--yellow)',
      },
    ],
    utilization: {
      ...mock.utilization,
      capacity: HISTORY_CAPACITY,
      series: [
        { name: 'Usage', color: 'var(--green)', fill: true, values: history.usagePercent },
        { name: 'Clock', color: 'var(--orange)', values: clockSeries },
        { name: 'Temperature', color: 'var(--yellow)', values: temperatureSeries },
      ],
    },
    topProcesses: {
      title: `Top CPU processes (${rows.length})`,
      columns: mock.topProcesses.columns,
      rows: rows.map((r) => ({
        pid: r.pid,
        name: r.name,
        cpu: r.cpuPercent.toFixed(2),
        memory: formatBytes(r.memoryBytes),
        cpuShade: cpuShade(r.cpuPercent, maxCpu),
        self: r.name.toLowerCase().startsWith(SELF_NAME_PREFIX),
      })),
    },
    summary: {
      metrics: [
        { label: 'Utilization', value: formatPercent(usage) },
        { label: 'Speed', value: formatGhz(speed) },
        { label: 'Temperature', value: formatCelsius(temperature) },
        { label: 'Processes', value: formatInteger(current?.processes) },
        { label: 'Threads', value: formatInteger(current?.threads) },
        { label: 'Handles', value: formatInteger(current?.handles) },
      ],
      uptime: {
        label: 'Up time', value: formatUptime(current?.uptimeSeconds),
        startedLabel: 'Started', started: formatDateTime(current?.bootTime),
      },
    },
    specifications: { title: 'Specifications', columns: specifications(info) },
    breakdown: {
      ...mock.breakdown,
      parts: [
        { name: 'User', percent: partPercent(b?.userPercent), color: 'var(--green)', width: widths.parts[0] },
        { name: 'Kernel', percent: partPercent(b?.kernelPercent), color: 'var(--orange)', width: widths.parts[1] },
        { name: 'DPC', percent: partPercent(b?.dpcPercent), color: 'var(--purple)', width: widths.parts[2] },
        { name: 'Interrupt', percent: partPercent(b?.interruptPercent), color: 'var(--yellow)', width: widths.parts[3] },
        { name: 'Idle', percent: partPercent(b?.idlePercent), color: 'var(--idle)', width: widths.idle },
      ],
      capacity: HISTORY_CAPACITY,
      trend: [
        { name: 'User', color: 'var(--green)', fill: true, values: history.userPercent },
        { name: 'Kernel', color: 'var(--orange)', values: history.kernelPercent },
      ],
    },
    scheduler: {
      ...mock.scheduler,
      metrics: [
        { label: 'Context switches', value: formatInteger(sc.contextSwitchesPerSec) },
        { label: 'Interrupts', value: formatInteger(sc.interruptsPerSec) },
        { label: 'System calls', value: formatInteger(sc.systemCallsPerSec) },
        { label: 'DPCs queued', value: formatInteger(sc.dpcsPerSec) },
      ],
      source: info?.countersSource || PLACEHOLDER,
    },
    perCore: (current?.perCoreUsagePercent ?? []).map((value, index) => ({
      label: `CPU ${index}`,
      value: formatPercent(value),
      series: perCoreHistory[index] ?? [],
    })),
    tooltips: mock.tooltips,
  };
}
