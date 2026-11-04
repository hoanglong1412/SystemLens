/**
 * @file   cpuPerformanceView.js
 * @brief  Performance > CPU screen: resource sidebar and the CPU detail panel (gauges, utilization
 *         chart, top processes, summary, specifications, CPU time breakdown, scheduler activity) with
 *         Overview / Per-core sub-tabs.
 *
 * The static frame (cards, titles, info icons) is built once; update() only refills the data slots.
 * Rebuilding everything every second would recreate the info icons, leaving a tooltip stuck open
 * when its icon disappears under the mouse.
 * Data comes from cpuPerformanceModel.js (shape of CPU_PERFORMANCE_MOCK plus perCore).
 */
import { dotted, h } from '../../ui/dom.js';
import { infoIcon } from '../../ui/tooltip.js';
import { plot } from '../../ui/charts/plot.js';
import { ledGauge } from '../../ui/charts/ledGauge.js';

/** Plot sizes from the design (outer size, border included). */
const SPARKLINE = { width: 72, height: 52, inset: 1 };
const UTILIZATION_PLOT = { width: 588, height: 228, grid: { columns: 20, rows: 10 } };
const TREND_PLOT = { width: 584, height: 52, grid: { columns: 20, rows: 0 } };
const CORE_PLOT = { width: 254, height: 96, grid: { columns: 10, rows: 4 } };

const SUB_TAB_OVERVIEW = 'Overview';
const SUB_TAB_PER_CORE = 'Per-core';

/**
 * Card container with the common border/background.
 * @param {string} modifier BEM modifier for the card's own layout.
 * @param {Array<Node>} children
 * @returns {HTMLElement}
 */
function card(modifier, children) {
  return h('section', { class: `card card--${modifier}`, 'data-testid': `card-${modifier}` }, children);
}

/**
 * Card title row: title text followed by its info icon.
 * @param {string|Node} title Text, or a slot element whose text changes.
 * @param {object} tip Tooltip data for infoIcon.
 * @param {string} [extraClass]
 * @returns {HTMLElement}
 */
function cardTitle(title, tip, extraClass = '') {
  return h('div', { class: `card__title ${extraClass}`.trim() }, [title, infoIcon(tip)]);
}

/**
 * Chart legend: coloured swatch + name per series.
 * @param {Array<{name: string, color: string}>} series
 * @returns {HTMLElement}
 */
function legend(series) {
  return h('div', { class: 'legend' }, series.map(({ name, color }) => {
    const swatch = h('span', { class: 'legend__swatch' });
    swatch.style.background = color;
    return h('span', { class: 'legend__item' }, [swatch, name]);
  }));
}

/**
 * One label/value metric.
 * @param {{label: string, value: string}} metric
 * @returns {HTMLElement}
 */
function metric({ label, value }) {
  return h('div', { class: 'metric', 'data-metric': label }, [
    h('div', { class: 'metric__label', text: label }),
    h('div', { class: 'metric__value', text: value }),
  ]);
}

/**
 * Sidebar rows: one per monitored resource with its sparkline.
 * @param {object} data View-model.
 * @returns {HTMLElement[]}
 */
function resourceRows(data) {
  return data.resources.map((res) => {
    const spark = plot({
      ...SPARKLINE, series: [{ values: res.series, color: res.color, fill: res.fill }], className: 'sparkline',
      capacity: data.utilization.capacity,
    });
    spark.style.color = res.color;
    return h('div', {
      class: res.selected ? 'resource resource--selected' : 'resource', 'data-testid': `resource-${res.id}`,
    }, [
      spark,
      h('div', { class: 'resource__text' }, [
        h('div', { class: 'resource__name', text: res.name }),
        h('div', { class: 'resource__detail' }, dotted(res.detail)),
      ]),
    ]);
  });
}

/**
 * Top processes table rows.
 * @param {object} t View-model topProcesses.
 * @returns {HTMLElement[]}
 */
function processRows(t) {
  return t.rows.map((row) => {
    const cpu = h('td', { class: 'proc__cpu', text: row.cpu });
    // backgroundColor, not the background shorthand: the shorthand would reset background-clip.
    cpu.style.backgroundColor = row.cpuShade;
    return h('tr', {}, [
      h('td', { class: 'proc__pid', text: String(row.pid) }),
      h('td', { class: row.self ? 'proc__name proc__name--self' : 'proc__name', text: row.name, title: row.name }),
      cpu,
      h('td', { class: 'proc__mem', text: row.memory }),
    ]);
  });
}

/**
 * Breakdown bar segments and legend items.
 * @param {object} b View-model breakdown.
 * @returns {{segments: HTMLElement[], items: HTMLElement[]}}
 */
function breakdownParts(b) {
  const segments = b.parts.map(({ name, color, width }) => {
    const seg = h('span', { class: 'breakdown__seg', 'data-part': name });
    seg.style.background = color;
    seg.style.width = `${width}px`;
    return seg;
  });
  const items = b.parts.map(({ name, percent, color }) => {
    const swatch = h('span', { class: 'breakdown__swatch' });
    swatch.style.background = color;
    return h('div', { class: 'breakdown__item', 'data-part': name }, [
      swatch,
      h('div', {}, [
        h('div', { class: 'breakdown__name', text: name }),
        h('div', { class: 'breakdown__percent', text: percent }),
      ]),
    ]);
  });
  return { segments, items };
}

/**
 * Per-core grid tiles.
 * @param {object} data View-model.
 * @returns {HTMLElement[]}
 */
function coreTiles(data) {
  return data.perCore.map((core) => h('div', { class: 'core-tile', 'data-testid': 'core-tile' }, [
    h('div', { class: 'core-tile__head' }, [
      h('span', { class: 'core-tile__label', text: core.label }),
      h('span', { class: 'core-tile__value', text: core.value }),
    ]),
    plot({
      ...CORE_PLOT, series: [{ values: core.series, color: 'var(--green)', fill: true }],
      className: 'chart__plot core-tile__plot', capacity: data.utilization.capacity,
    }),
  ]));
}

/**
 * Render the CPU performance screen and return a handle to refresh it.
 * @param {HTMLElement} container Shell content slot (emptied first).
 * @param {object} initial View-model (see cpuPerformanceModel.js).
 * @returns {{update: (data: object) => void, showSubTab: (name: string) => void}}
 */
export function createCpuPerformanceView(container, initial) {
  const tips = initial.tooltips;
  let latest = initial;
  let activeSubTab = SUB_TAB_OVERVIEW;

  // ---- Slots: elements whose content changes on update() ----
  const resourcesList = h('div', { class: 'resources__list' });
  const model = h('div', { class: 'cpu-header__model', 'data-testid': 'cpu-model' });
  const topology = h('div', { class: 'cpu-header__topology', 'data-testid': 'cpu-topology' });
  const gauges = h('div', { class: 'gauges' });
  const utilizationPlot = h('div', { class: 'chart__plot-slot' });
  const procTitle = h('span', {});
  const procBody = h('tbody', {});
  const summaryMetrics = h('div', { class: 'summary__metrics' });
  const uptimeValue = h('div', { class: 'summary__uptime-value', 'data-testid': 'uptime' });
  const started = h('div', { class: 'summary__started', 'data-testid': 'started' });
  const spec = h('div', { class: 'spec' });
  const breakdownBar = h('div', { class: 'breakdown__bar' });
  const breakdownLegend = h('div', { class: 'breakdown__legend' });
  const trendPlot = h('div', { class: 'chart__plot-slot' });
  const schedulerMetrics = h('div', { class: 'scheduler__metrics' });
  const schedulerSource = h('div', { class: 'scheduler__source' });
  const coreGrid = h('div', { class: 'core-grid', 'data-testid': 'core-grid' });

  const subTabButtons = initial.header.subTabs.map((name) => h('button', {
    type: 'button', class: 'subtab', text: name, 'data-testid': `subtab-${name}`,
    on: { click: () => showSubTab(name) },
  }));

  const overview = h('div', { class: 'cpu__overview' }, [
    h('div', { class: 'cpu__row cpu__row--top' }, [
      card('gauges', [cardTitle('Live gauges', tips.gauges, 'card__title--small'), gauges]),
      card('utilization', [
        h('div', { class: 'chart__top' }, [
          cardTitle(initial.utilization.title, tips.utilization, 'card__title--small'),
          h('span', { class: 'chart__axis', text: initial.utilization.max }),
        ]),
        utilizationPlot,
        h('div', { class: 'chart__bottom' }, [
          h('span', { class: 'chart__axis', text: initial.utilization.xStart }),
          legend(initial.utilization.series),
          h('span', { class: 'chart__axis', text: initial.utilization.xEnd }),
        ]),
      ]),
      card('processes', [
        cardTitle(h('span', {}, ['» ', procTitle]), tips.topProcesses, 'card__title--table'),
        h('table', { class: 'proc', 'data-testid': 'top-processes' }, [
          h('thead', {}, [h('tr', {}, initial.topProcesses.columns.map((c) => h('th', { text: c })))]),
          procBody,
        ]),
      ]),
    ]),
    h('div', { class: 'cpu__row cpu__row--mid' }, [
      card('summary', [
        summaryMetrics,
        h('div', { class: 'summary__divider' }),
        h('div', { class: 'summary__uptime' }, [
          h('div', { class: 'metric__label', text: initial.summary.uptime.label }),
          uptimeValue,
          h('div', { class: 'metric__label summary__started-label', text: initial.summary.uptime.startedLabel }),
          started,
        ]),
        infoIcon(tips.summary, 'summary__info'),
      ]),
      card('spec', [cardTitle(initial.specifications.title, tips.specifications), spec]),
    ]),
    h('div', { class: 'cpu__row cpu__row--bottom' }, [
      card('breakdown', [
        h('div', { class: 'card__head' }, [
          cardTitle(initial.breakdown.title, tips.breakdown),
          h('span', { class: 'card__aside' }, dotted(initial.breakdown.scope)),
        ]),
        breakdownBar,
        breakdownLegend,
        h('div', { class: 'breakdown__caption' }, dotted(initial.breakdown.trendCaption)),
        trendPlot,
      ]),
      card('scheduler', [
        h('div', { class: 'card__head' }, [
          cardTitle(initial.scheduler.title, tips.scheduler),
          h('span', { class: 'card__aside', text: initial.scheduler.unit }),
        ]),
        schedulerMetrics,
        schedulerSource,
      ]),
    ]),
  ]);

  container.replaceChildren(h('div', { class: 'perf', 'data-testid': 'cpu-view' }, [
    h('aside', { class: 'resources' }, [infoIcon(tips.resources, 'resources__info'), resourcesList]),
    h('div', { class: 'cpu' }, [
      h('div', { class: 'cpu-header' }, [
        h('h1', { class: 'cpu-header__title', text: initial.header.title }),
        model,
        topology,
        h('div', { class: 'subtabs' }, subTabButtons),
      ]),
      overview,
      coreGrid,
    ]),
  ]));

  /**
   * Refill every data slot.
   * @param {object} data View-model.
   */
  function update(data) {
    latest = data;
    resourcesList.replaceChildren(...resourceRows(data));
    model.textContent = data.header.model;
    topology.replaceChildren(...dotted(data.header.topology));
    gauges.replaceChildren(...data.gauges.map(ledGauge));
    utilizationPlot.replaceChildren(plot({
      ...UTILIZATION_PLOT, series: data.utilization.series, className: 'chart__plot',
      capacity: data.utilization.capacity,
    }));
    procTitle.textContent = data.topProcesses.title;
    procBody.replaceChildren(...processRows(data.topProcesses));
    summaryMetrics.replaceChildren(...data.summary.metrics.map(metric));
    uptimeValue.textContent = data.summary.uptime.value;
    started.textContent = data.summary.uptime.started;
    spec.replaceChildren(...data.specifications.columns.map((rows) =>
      h('div', { class: 'spec__column' }, rows.map(([label, value]) =>
        h('div', { class: 'spec__row', 'data-spec': label }, [
          h('span', { class: 'spec__label', text: label }),
          h('span', { class: 'spec__value', text: value }),
        ])))));
    const { segments, items } = breakdownParts(data.breakdown);
    breakdownBar.replaceChildren(...segments);
    breakdownLegend.replaceChildren(...items);
    trendPlot.replaceChildren(plot({
      ...TREND_PLOT, series: data.breakdown.trend, className: 'chart__plot chart__plot--trend',
      capacity: data.breakdown.capacity,
    }));
    schedulerMetrics.replaceChildren(...data.scheduler.metrics.map(metric));
    schedulerSource.textContent = data.scheduler.source;
    // The per-core grid is only rebuilt while visible: it holds one chart per logical processor.
    if (activeSubTab === SUB_TAB_PER_CORE) coreGrid.replaceChildren(...coreTiles(data));
  }

  /**
   * Switch between the overview cards and the per-core grid.
   * @param {string} name Sub-tab name ("Overview" or "Per-core").
   */
  function showSubTab(name) {
    activeSubTab = name;
    subTabButtons.forEach((b) => b.classList.toggle('subtab--active', b.textContent === name));
    const perCore = name === SUB_TAB_PER_CORE;
    overview.classList.toggle('cpu__overview--hidden', perCore);
    coreGrid.classList.toggle('core-grid--visible', perCore);
    if (perCore) coreGrid.replaceChildren(...coreTiles(latest));
  }

  update(initial);
  showSubTab(SUB_TAB_OVERVIEW);
  return { update, showSubTab };
}
