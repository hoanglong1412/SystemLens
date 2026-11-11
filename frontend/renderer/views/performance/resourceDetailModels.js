/**
 * @file   resourceDetailModels.js
 * @brief  Turn the Memory / Disk / Network / GPU API objects into the display model of the generic
 *         resource detail panel (resourceDetailView.js). No DOM here.
 *
 * Model shape: {title, subtitle, charts: [{title, tooltip, topRight, legend, series}], statsTooltip,
 * stats: [{label, value}], info: {title, tooltip, rows: [[label, value]]},
 * table: null | {title, tooltip, columns: [{label, align}], rows: [{cells, self}]}}
 */
import {
  NO_VALUE, formatBitRate, formatByteRate, formatBytes, formatPercent, formatTemperature,
} from '../../ui/format.js';
import { padHistory, scaleToPeak } from '../../ui/series.js';
import { DETAIL_TEXTS } from './resourceDetailTexts.js';

/** Smallest full scale of a transfer chart, so an idle device does not amplify noise. */
const MIN_DISK_SCALE_BYTES_PER_SECOND = 100 * 1024;
const MIN_NETWORK_SCALE_BITS_PER_SECOND = 100_000;

/**
 * @param {string} name Series name.
 * @param {string} color CSS colour.
 * @returns {{name: string, color: string}} Legend entry.
 */
function legendEntry(name, color) {
  return { name, color };
}

/**
 * Model of the Memory panel.
 * @param {object|null} memory MemoryPerformance from the API; null before the first response.
 * @returns {object}
 */
export function toMemoryModel(memory) {
  const t = DETAIL_TEXTS.memory;
  const color = 'var(--blue)';
  const committed = memory?.committedBytes ?? null;
  return {
    title: t.title,
    subtitle: memory ? formatBytes(memory.totalBytes) : NO_VALUE,
    charts: [{
      title: t.chartTitle,
      tooltip: t.tooltips.chart,
      topRight: DETAIL_TEXTS.fullScale,
      legend: [legendEntry(t.legendInUse, color)],
      series: [{ values: padHistory(memory?.history, 0), color, fill: true }],
    }],
    statsTooltip: t.tooltips.stats,
    stats: [
      { label: t.labels.inUse, value: memory ? formatBytes(memory.usedBytes) : NO_VALUE },
      { label: t.labels.available, value: memory ? formatBytes(memory.availableBytes) : NO_VALUE },
      { label: t.labels.cached, value: formatBytes(memory?.cachedBytes ?? null) },
      { label: t.labels.committed, value: formatBytes(committed) },
      { label: t.labels.usage, value: formatPercent(memory?.usedPercent ?? null) },
    ],
    info: {
      title: t.infoTitle,
      tooltip: t.tooltips.info,
      rows: [
        [t.infoLabels.total, memory ? formatBytes(memory.totalBytes) : NO_VALUE],
        [t.infoLabels.commitLimit, formatBytes(memory?.commitLimitBytes ?? null)],
        [t.infoLabels.usage, formatPercent(memory?.usedPercent ?? null, 1)],
      ],
    },
    table: {
      title: t.tableTitle,
      tooltip: t.tooltips.table,
      columns: t.tableColumns.map((label, index) => ({ label, align: index === 1 ? 'left' : 'right' })),
      rows: (memory?.topProcesses ?? []).map((process) => ({
        cells: [String(process.pid), process.name, formatBytes(process.memoryBytes), process.cpuPercent.toFixed(1)],
        self: process.isSelf,
      })),
    },
  };
}

/**
 * Model of a Disk panel.
 * @param {object[]|null} disks DiskDetail[] from the API; null before the first response.
 * @param {string} id Id of the selected disk.
 * @returns {object}
 */
export function toDiskModel(disks, id) {
  const t = DETAIL_TEXTS.disk;
  const disk = (disks ?? []).find((item) => item.id === id);
  const { scaled, peak } = scaleToPeak([disk?.readHistory, disk?.writeHistory], MIN_DISK_SCALE_BYTES_PER_SECOND);
  return {
    title: disk?.name ?? NO_VALUE,
    subtitle: disk?.model ?? (disk ? t.kinds[disk.kind] : NO_VALUE),
    charts: [
      {
        title: t.chartActive,
        tooltip: t.tooltips.active,
        topRight: DETAIL_TEXTS.fullScale,
        legend: [legendEntry(t.chartActive, 'var(--lime)')],
        series: [{ values: padHistory(disk?.activeHistory, 0), color: 'var(--lime)', fill: true }],
      },
      {
        title: t.chartTransfer,
        tooltip: t.tooltips.transfer,
        topRight: `${DETAIL_TEXTS.peakPrefix} ${formatByteRate(peak)}`,
        legend: [legendEntry(t.legendRead, 'var(--green)'), legendEntry(t.legendWrite, 'var(--orange)')],
        series: [
          { values: scaled[0], color: 'var(--green)', fill: true },
          { values: scaled[1], color: 'var(--orange)' },
        ],
      },
    ],
    statsTooltip: t.tooltips.stats,
    stats: [
      { label: t.labels.active, value: formatPercent(disk?.activePercent ?? null) },
      { label: t.labels.read, value: formatByteRate(disk?.readBytesPerSecond ?? null) },
      { label: t.labels.write, value: formatByteRate(disk?.writeBytesPerSecond ?? null) },
    ],
    info: {
      title: t.infoTitle,
      tooltip: t.tooltips.info,
      rows: [
        [t.infoLabels.model, disk?.model ?? NO_VALUE],
        [t.infoLabels.type, disk ? t.kinds[disk.kind] : NO_VALUE],
        [t.infoLabels.capacity, formatBytes(disk?.capacityBytes ?? null)],
        [t.infoLabels.device, disk?.name ?? NO_VALUE],
      ],
    },
    table: null,
  };
}

/**
 * Model of an Ethernet / Wi-Fi panel.
 * @param {object[]|null} adapters NetworkDetail[] from the API; null before the first response.
 * @param {string} id Id of the selected adapter.
 * @returns {object}
 */
export function toNetworkModel(adapters, id) {
  const t = DETAIL_TEXTS.network;
  const adapter = (adapters ?? []).find((item) => item.id === id);
  const kindName = adapter?.kind === 'wifi' ? t.wifi : t.ethernet;
  const { scaled, peak } = scaleToPeak([adapter?.sendHistory, adapter?.receiveHistory], MIN_NETWORK_SCALE_BITS_PER_SECOND);
  return {
    title: adapter ? kindName : NO_VALUE,
    subtitle: adapter?.description ?? adapter?.name ?? NO_VALUE,
    charts: [{
      title: t.chartTitle,
      tooltip: t.tooltips.chart,
      topRight: `${DETAIL_TEXTS.peakPrefix} ${formatBitRate(peak)}`,
      legend: [legendEntry(t.legendSend, 'var(--orange)'), legendEntry(t.legendReceive, 'var(--blue)')],
      series: [
        { values: scaled[0], color: 'var(--orange)', fill: true },
        { values: scaled[1], color: 'var(--blue)' },
      ],
    }],
    statsTooltip: t.tooltips.stats,
    stats: [
      { label: t.labels.send, value: formatBitRate(adapter?.sendBitsPerSecond ?? null) },
      { label: t.labels.receive, value: formatBitRate(adapter?.receiveBitsPerSecond ?? null) },
      { label: t.labels.linkSpeed, value: formatBitRate(adapter?.linkSpeedBitsPerSecond ?? null) },
    ],
    info: {
      title: t.infoTitle,
      tooltip: t.tooltips.info,
      rows: [
        [t.infoLabels.name, adapter?.name ?? NO_VALUE],
        [t.infoLabels.description, adapter?.description ?? NO_VALUE],
        [t.infoLabels.type, adapter ? kindName : NO_VALUE],
        [t.infoLabels.mac, adapter?.macAddress ?? NO_VALUE],
        [t.infoLabels.linkSpeed, formatBitRate(adapter?.linkSpeedBitsPerSecond ?? null)],
      ],
    },
    table: null,
  };
}

/**
 * Model of a GPU panel.
 * @param {object[]|null} gpus GpuDetail[] from the API; null before the first response.
 * @param {string} id Id of the selected adapter.
 * @returns {object}
 */
export function toGpuModel(gpus, id) {
  const t = DETAIL_TEXTS.gpu;
  const gpu = (gpus ?? []).find((item) => item.id === id);
  const color = 'var(--purple)';
  return {
    title: gpu?.name ?? NO_VALUE,
    subtitle: gpu?.model ?? NO_VALUE,
    charts: [{
      title: t.chartTitle,
      tooltip: t.tooltips.chart,
      topRight: DETAIL_TEXTS.fullScale,
      legend: [legendEntry(t.chartTitle, color)],
      series: [{ values: padHistory(gpu?.history, 0), color, fill: true }],
    }],
    statsTooltip: t.tooltips.stats,
    stats: [
      { label: t.labels.utilization, value: formatPercent(gpu?.utilizationPercent ?? null) },
      { label: t.labels.temperature, value: formatTemperature(gpu?.temperatureCelsius ?? null) },
      { label: t.labels.memoryUsed, value: formatBytes(gpu?.memoryUsedBytes ?? null) },
    ],
    info: {
      title: t.infoTitle,
      tooltip: t.tooltips.info,
      rows: [
        [t.infoLabels.model, gpu?.model ?? NO_VALUE],
        [t.infoLabels.memory, formatBytes(gpu?.memoryTotalBytes ?? null)],
        [t.infoLabels.name, gpu?.name ?? NO_VALUE],
      ],
    },
    table: null,
  };
}
