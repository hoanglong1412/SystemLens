/**
 * @file   resourceDetailTexts.js
 * @brief  Static texts of the Memory, Disk, Network and GPU panels: titles, labels and tooltips.
 */
import { Placement } from '../../ui/tooltip.js';

export const DETAIL_TEXTS = Object.freeze({
  xStart: '60 seconds',
  xEnd: '0',
  fullScale: '100%',
  peakPrefix: 'Peak',
  memory: {
    title: 'Memory',
    chartTitle: 'Memory usage',
    legendInUse: 'In use',
    labels: { inUse: 'In use', available: 'Available', cached: 'Cached', committed: 'Committed', usage: 'Usage' },
    infoTitle: 'Details',
    infoLabels: { total: 'Total memory', commitLimit: 'Commit limit', usage: 'Usage' },
    tableTitle: 'Top memory processes',
    tableColumns: ['PID', 'Name', 'Memory', 'CPU %'],
    tooltips: {
      chart: { title: 'Memory usage', body: 'Share of physical memory in use over the last 60 seconds, sampled every second.' },
      stats: { title: 'Memory figures', body: 'In use = total minus available; cached = file cache that can be dropped; committed = memory the system has promised to programs.' },
      info: { title: 'Details', body: 'Installed physical memory and the commit limit (physical memory plus swap / page file).', placement: Placement.BOTTOM_END },
      table: { title: 'Top memory processes', body: 'The 10 processes using the most memory (private working set on Windows, resident set on Linux).' },
    },
  },
  disk: {
    chartActive: 'Active time',
    chartTransfer: 'Transfer rate',
    legendRead: 'Read',
    legendWrite: 'Write',
    labels: { active: 'Active time', read: 'Read speed', write: 'Write speed' },
    infoTitle: 'Disk',
    infoLabels: { model: 'Model', type: 'Type', capacity: 'Capacity', device: 'Device' },
    kinds: { ssd: 'SSD', hdd: 'HDD', unknown: 'Unknown' },
    tooltips: {
      active: { title: 'Active time', body: 'Share of time the disk had requests in progress over the last 60 seconds.' },
      transfer: { title: 'Transfer rate', body: 'Bytes read from and written to the disk per second, scaled to the peak of the last 60 seconds.' },
      stats: { title: 'Disk figures', body: 'Current active time and transfer speeds.' },
      info: { title: 'Disk', body: 'Product name, media type and size of the physical disk.', placement: Placement.BOTTOM_END },
    },
  },
  network: {
    ethernet: 'Ethernet',
    wifi: 'Wi-Fi',
    chartTitle: 'Throughput',
    legendSend: 'Send',
    legendReceive: 'Receive',
    labels: { send: 'Send', receive: 'Receive', linkSpeed: 'Link speed' },
    infoTitle: 'Adapter',
    infoLabels: { name: 'Name', description: 'Description', type: 'Type', mac: 'MAC address', linkSpeed: 'Link speed' },
    tooltips: {
      chart: { title: 'Throughput', body: 'Bits sent and received per second over the last 60 seconds, scaled to the peak of the window.' },
      stats: { title: 'Network figures', body: 'Current send and receive rate and the speed the link negotiated.' },
      info: { title: 'Adapter', body: 'Name, description and hardware address of the network adapter.', placement: Placement.BOTTOM_END },
    },
  },
  gpu: {
    chartTitle: 'Utilization',
    labels: { utilization: 'Utilization', temperature: 'Temperature', memoryUsed: 'Memory used' },
    infoTitle: 'Adapter',
    infoLabels: { model: 'Model', memory: 'Dedicated memory', name: 'Name' },
    tooltips: {
      chart: { title: 'Utilization', body: 'Load of the busiest engine of the graphics adapter over the last 60 seconds. Not every driver reports it.' },
      stats: { title: 'GPU figures', body: 'Current utilization, temperature and video memory in use. Values the driver does not report show a dash.' },
      info: { title: 'Adapter', body: 'Model and video memory of the graphics adapter.', placement: Placement.BOTTOM_END },
    },
  },
});
