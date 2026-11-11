/**
 * @file   performanceScreen.js
 * @brief  The Performance tab: resource sidebar on the left and, on the right, the panel of the selected
 *         resource (CPU, Memory, Disk, Ethernet / Wi-Fi, GPU). Refreshes once per second while visible.
 *
 * Only the data of the selected resource is requested (plus the sidebar list). Selecting another
 * resource switches the panel at once and reloads without waiting for the timer.
 */
import { h } from '../../ui/dom.js';
import { createPollingScreen } from '../../live/pollingScreen.js';
import {
  getCpuCores, getCpuPerformance, getDiskPerformance, getGpuPerformance, getMemoryPerformance,
  getNetworkPerformance, getPerformanceResources,
} from '../../api/performance/performanceApi.js';
import { CPU_TEXTS } from '../cpu/cpuPerformanceTexts.js';
import { toCpuViewData } from '../cpu/cpuPerformanceModel.js';
import { createCpuPerformanceView } from '../cpu/cpuPerformanceView.js';
import { DEFAULT_RESOURCE_ID, toResourceRows } from './resourceModel.js';
import { createResourceSidebar } from './resourceSidebar.js';
import { createResourceDetailView } from './resourceDetailView.js';
import { toDiskModel, toGpuModel, toMemoryModel, toNetworkModel } from './resourceDetailModels.js';

/** Pause between two refreshes. */
const REFRESH_INTERVAL_MS = 1000;

/**
 * Which panel shows a resource. Ids come from the backend: "cpu", "memory", "disk<N>", "net-<name>", "gpu<N>".
 * @param {string} id Resource id.
 * @returns {'cpu'|'memory'|'disk'|'network'|'gpu'}
 */
function kindOf(id) {
  if (id === 'memory') return 'memory';
  if (id.startsWith('disk')) return 'disk';
  if (id.startsWith('net-')) return 'network';
  if (id.startsWith('gpu')) return 'gpu';
  return 'cpu';
}

/** Loader of the panel data per kind; each returns what modelFor() needs. */
const DETAIL_LOADERS = Object.freeze({
  cpu: (client) => Promise.all([getCpuPerformance(client), getCpuCores(client)]),
  memory: getMemoryPerformance,
  disk: getDiskPerformance,
  network: getNetworkPerformance,
  gpu: getGpuPerformance,
});

/**
 * Display model of a panel.
 * @param {string} kind Panel kind.
 * @param {string} id Selected resource id.
 * @param {any} detail Result of the kind's loader; null before the first response.
 * @returns {object}
 */
function modelFor(kind, id, detail) {
  switch (kind) {
    case 'memory': return toMemoryModel(detail);
    case 'disk': return toDiskModel(detail, id);
    case 'network': return toNetworkModel(detail, id);
    case 'gpu': return toGpuModel(detail, id);
    default: return toCpuViewData(detail?.[0] ?? null, detail?.[1] ?? null);
  }
}

/**
 * Create the Performance screen definition for the ViewSwitcher.
 * @param {object} options
 * @param {() => Promise<import('../../api/apiClient.js').ApiClient>} options.getClient
 * @returns {{render: Function, onShow: Function, onHide: Function}}
 */
export function createPerformanceScreen({ getClient }) {
  let selectedId = DEFAULT_RESOURCE_ID;
  let lastResources = null;
  let sidebar = null;
  let panelHost = null;
  const panels = new Map();
  let shownKind = null;

  /**
   * Show the panel of a kind, creating it on first use.
   * @param {string} kind Panel kind.
   * @returns {{element: HTMLElement, update: (model: object) => void}}
   */
  function showPanel(kind) {
    if (!panels.has(kind)) {
      panels.set(kind, kind === 'cpu'
        ? createCpuPerformanceView(toCpuViewData(null, null))
        : createResourceDetailView());
    }
    const panel = panels.get(kind);
    if (shownKind !== kind) {
      panelHost.replaceChildren(panel.element);
      shownKind = kind;
    }
    return panel;
  }

  /**
   * The user picked a resource: switch the panel now, load its data without waiting for the timer.
   * @param {string} id Resource id.
   */
  function select(id) {
    selectedId = id;
    sidebar.update(toResourceRows(lastResources, selectedId));
    showPanel(kindOf(selectedId));
    screen.refresh();
  }

  const screen = createPollingScreen({
    getClient,
    intervalMs: REFRESH_INTERVAL_MS,
    load: async (client) => {
      const id = selectedId;
      const kind = kindOf(id);
      const [resources, detail] = await Promise.all([getPerformanceResources(client), DETAIL_LOADERS[kind](client)]);
      return { id, kind, resources, detail };
    },
    create: (host) => {
      sidebar = createResourceSidebar({ tooltip: CPU_TEXTS.tooltips.resources, onSelect: select });
      panelHost = h('div', { class: 'perf__panel' });
      host.replaceChildren(h('div', { class: 'perf' }, [sidebar.element, panelHost]));
      sidebar.update([]);
      showPanel(kindOf(selectedId));
      return {
        update({ id, kind, resources, detail }) {
          // The user picked another resource while this request was running: its data is stale.
          if (id !== selectedId) return;
          lastResources = resources;
          if (!resources.some((resource) => resource.id === id)) {
            // The resource disappeared (e.g. a removed disk): fall back to the CPU.
            select(DEFAULT_RESOURCE_ID);
            return;
          }
          sidebar.update(toResourceRows(resources, id));
          showPanel(kind).update(modelFor(kind, id, detail));
        },
      };
    },
  });
  return screen;
}
