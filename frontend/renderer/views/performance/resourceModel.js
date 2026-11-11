/**
 * @file   resourceModel.js
 * @brief  Turns the PerformanceResource[] API objects into the rows of the resource sidebar
 *         (name, detail text, sparkline). No DOM here.
 */
import {
  NO_VALUE, formatBitRate, formatFrequency, formatGigabytesValue, formatPercent, formatTemperature,
} from '../../ui/format.js';
import { padHistory } from '../../ui/series.js';

/** Resource selected when the screen opens. */
export const DEFAULT_RESOURCE_ID = 'cpu';

/** Sparkline colour per resource kind. */
const RESOURCE_COLORS = Object.freeze({
  cpu: 'var(--green)',
  memory: 'var(--blue)',
  disk: 'var(--lime)',
  ethernet: 'var(--orange-dim)',
  wifi: 'var(--orange-dark)',
  gpu: 'var(--purple)',
});

/**
 * Detail line of a sidebar resource, depending on its kind.
 * @param {object} resource PerformanceResource.
 * @returns {string[]} Parts joined with "·" by the view.
 */
function resourceDetail(resource) {
  const percent = formatPercent(resource.percent);
  switch (resource.kind) {
    case 'cpu':
      return resource.frequencyMhz === null ? [percent] : [percent, formatFrequency(resource.frequencyMhz)];
    case 'memory':
      return [`${formatGigabytesValue(resource.usedBytes)}/${formatGigabytesValue(resource.totalBytes)} GB (${percent})`];
    case 'disk':
      return resource.description ? [resource.description, percent] : [percent];
    case 'ethernet':
      return [
        resource.description ?? NO_VALUE,
        formatBitRate((resource.sendBitsPerSecond ?? 0) + (resource.receiveBitsPerSecond ?? 0)),
      ];
    case 'wifi':
      return [`S ${formatBitRate(resource.sendBitsPerSecond)}  R ${formatBitRate(resource.receiveBitsPerSecond)}`];
    case 'gpu': {
      const load = resource.temperatureCelsius === null
        ? percent
        : `${percent} (${formatTemperature(resource.temperatureCelsius)})`;
      return [resource.description ?? NO_VALUE, load];
    }
    default:
      return [percent];
  }
}

/**
 * Sidebar rows.
 * @param {object[]|null} resources PerformanceResource[]; null before the first response.
 * @param {string} selectedId Id of the selected resource.
 * @returns {Array<{id: string, name: string, detail: string[], color: string, fill: boolean, selected: boolean, series: number[]}>}
 */
export function toResourceRows(resources, selectedId) {
  return (resources ?? []).map((resource) => ({
    id: resource.id,
    name: resource.name,
    detail: resourceDetail(resource),
    color: RESOURCE_COLORS[resource.kind] ?? RESOURCE_COLORS.cpu,
    fill: true,
    selected: resource.id === selectedId,
    series: padHistory(resource.history, 0),
  }));
}
