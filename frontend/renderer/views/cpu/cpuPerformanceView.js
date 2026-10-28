/**
 * @file   cpuPerformanceView.js
 * @brief  Performance > CPU screen: resource sidebar and the CPU detail panel (gauges, utilization
 *         chart, top processes, summary, specifications, CPU time breakdown, scheduler activity).
 *         Static for now: renders the data object it is given (CPU_PERFORMANCE_MOCK).
 */
import { dotted, h } from '../../ui/dom.js';
import { infoIcon } from '../../ui/tooltip.js';
import { plot } from '../../ui/charts/plot.js';
import { ledGauge } from '../../ui/charts/ledGauge.js';

/** Plot sizes from the design (outer size, border included). */
const SPARKLINE = { width: 72, height: 52, inset: 1 };
const UTILIZATION_PLOT = { width: 588, height: 228, grid: { columns: 20, rows: 10 } };
const TREND_PLOT = { width: 584, height: 52, grid: { columns: 20, rows: 0 } };

/**
 * Card container with the common border/background.
 * @param {string} modifier BEM modifier for the card's own layout.
 * @param {Array<Node>} children
 * @returns {HTMLElement}
 */
function card(modifier, children) {
  return h('section', { class: `card card--${modifier}` }, children);
}

/**
 * Card title row: title text followed by its info icon.
 * @param {string} title
 * @param {object} tip Tooltip data for infoIcon.
 * @param {string} [extraClass]
 * @returns {HTMLElement}
 */
function cardTitle(title, tip, extraClass = '') {
  return h('div', { class: `card__title ${extraClass}`.trim() }, [title, infoIcon(tip)]);
}

/**
 * Sidebar: one row per monitored resource with its sparkline.
 * @param {object} data CPU_PERFORMANCE_MOCK-shaped data.
 * @returns {HTMLElement}
 */
function resourceSidebar(data) {
  return h('aside', { class: 'resources' }, [
    infoIcon(data.tooltips.resources, 'resources__info'),
    ...data.resources.map((res) => {
      const spark = plot({ ...SPARKLINE, series: [{ values: res.series, color: res.color, fill: res.fill }], className: 'sparkline' });
      spark.style.color = res.color;
      return h('div', { class: res.selected ? 'resource resource--selected' : 'resource' }, [
        spark,
        h('div', { class: 'resource__text' }, [
          h('div', { class: 'resource__name', text: res.name }),
          h('div', { class: 'resource__detail' }, dotted(res.detail)),
        ]),
      ]);
    }),
  ]);
}

/**
 * CPU title, model, topology and Overview / Per-core sub-tabs.
 * @param {object} header
 * @returns {HTMLElement}
 */
function cpuHeader(header) {
  return h('div', { class: 'cpu-header' }, [
    h('h1', { class: 'cpu-header__title', text: header.title }),
    h('div', { class: 'cpu-header__model', text: header.model }),
    h('div', { class: 'cpu-header__topology' }, dotted(header.topology)),
    h('div', { class: 'subtabs' }, header.subTabs.map((name) =>
      h('button', { type: 'button', class: name === header.activeSubTab ? 'subtab subtab--active' : 'subtab', text: name }))),
  ]);
}

/**
 * Live gauges card.
 * @param {object} data
 * @returns {HTMLElement}
 */
function gaugesCard(data) {
  return card('gauges', [
    cardTitle('Live gauges', data.tooltips.gauges, 'card__title--small'),
    h('div', { class: 'gauges' }, data.gauges.map(ledGauge)),
  ]);
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
 * % Utilization chart card.
 * @param {object} data
 * @returns {HTMLElement}
 */
function utilizationCard(data) {
  const u = data.utilization;
  // The usage area is drawn first so the clock / temperature lines stay visible over it.
  const series = u.series.map(({ values, color, fill }) => ({ values, color, fill }));
  return card('utilization', [
    h('div', { class: 'chart__top' }, [
      cardTitle(u.title, data.tooltips.utilization, 'card__title--small'),
      h('span', { class: 'chart__axis', text: u.max }),
    ]),
    plot({ ...UTILIZATION_PLOT, series, className: 'chart__plot' }),
    h('div', { class: 'chart__bottom' }, [
      h('span', { class: 'chart__axis', text: u.xStart }),
      legend(u.series),
      h('span', { class: 'chart__axis', text: u.xEnd }),
    ]),
  ]);
}

/**
 * Top CPU processes table card.
 * @param {object} data
 * @returns {HTMLElement}
 */
function topProcessesCard(data) {
  const t = data.topProcesses;
  const rows = t.rows.map((row) => {
    const cpu = h('td', { class: 'proc__cpu', text: row.cpu });
    // backgroundColor, not the background shorthand: the shorthand would reset background-clip.
    cpu.style.backgroundColor = row.cpuShade;
    return h('tr', {}, [
      h('td', { class: 'proc__pid', text: String(row.pid) }),
      h('td', { class: row.self ? 'proc__name proc__name--self' : 'proc__name', text: row.name }),
      cpu,
      h('td', { class: 'proc__mem', text: row.memory }),
    ]);
  });
  return card('processes', [
    cardTitle(`» ${t.title}`, data.tooltips.topProcesses, 'card__title--table'),
    h('table', { class: 'proc' }, [
      h('thead', {}, [h('tr', {}, t.columns.map((c) => h('th', { text: c })))]),
      h('tbody', {}, rows),
    ]),
  ]);
}

/**
 * One label/value metric.
 * @param {{label: string, value: string}} metric
 * @returns {HTMLElement}
 */
function metric({ label, value }) {
  return h('div', { class: 'metric' }, [
    h('div', { class: 'metric__label', text: label }),
    h('div', { class: 'metric__value', text: value }),
  ]);
}

/**
 * Summary card: six headline metrics and the uptime block.
 * @param {object} data
 * @returns {HTMLElement}
 */
function summaryCard(data) {
  const { metrics, uptime } = data.summary;
  return card('summary', [
    h('div', { class: 'summary__metrics' }, metrics.map(metric)),
    h('div', { class: 'summary__divider' }),
    h('div', { class: 'summary__uptime' }, [
      h('div', { class: 'metric__label', text: uptime.label }),
      h('div', { class: 'summary__uptime-value', text: uptime.value }),
      h('div', { class: 'metric__label summary__started-label', text: uptime.startedLabel }),
      h('div', { class: 'summary__started', text: uptime.started }),
    ]),
    infoIcon(data.tooltips.summary, 'summary__info'),
  ]);
}

/**
 * Specifications card: two columns of label/value rows.
 * @param {object} data
 * @returns {HTMLElement}
 */
function specificationsCard(data) {
  const spec = data.specifications;
  return card('spec', [
    cardTitle(spec.title, data.tooltips.specifications),
    h('div', { class: 'spec' }, spec.columns.map((rows) =>
      h('div', { class: 'spec__column' }, rows.map(([label, value]) =>
        h('div', { class: 'spec__row' }, [
          h('span', { class: 'spec__label', text: label }),
          h('span', { class: 'spec__value', text: value }),
        ]))))),
  ]);
}

/**
 * CPU time breakdown card: stacked bar, legend with percentages and User vs Kernel trend.
 * @param {object} data
 * @returns {HTMLElement}
 */
function breakdownCard(data) {
  const b = data.breakdown;
  const segments = b.parts.map(({ color, width }) => {
    const seg = h('span', { class: 'breakdown__seg' });
    seg.style.background = color;
    seg.style.width = `${width}px`;
    return seg;
  });
  const items = b.parts.map(({ name, percent, color }) => {
    const swatch = h('span', { class: 'breakdown__swatch' });
    swatch.style.background = color;
    return h('div', { class: 'breakdown__item' }, [
      swatch,
      h('div', {}, [
        h('div', { class: 'breakdown__name', text: name }),
        h('div', { class: 'breakdown__percent', text: percent }),
      ]),
    ]);
  });
  return card('breakdown', [
    h('div', { class: 'card__head' }, [
      cardTitle(b.title, data.tooltips.breakdown),
      h('span', { class: 'card__aside' }, dotted(b.scope)),
    ]),
    h('div', { class: 'breakdown__bar' }, segments),
    h('div', { class: 'breakdown__legend' }, items),
    h('div', { class: 'breakdown__caption' }, dotted(b.trendCaption)),
    plot({ ...TREND_PLOT, series: b.trend, className: 'chart__plot chart__plot--trend' }),
  ]);
}

/**
 * Scheduler activity card.
 * @param {object} data
 * @returns {HTMLElement}
 */
function schedulerCard(data) {
  const sc = data.scheduler;
  return card('scheduler', [
    h('div', { class: 'card__head' }, [
      cardTitle(sc.title, data.tooltips.scheduler),
      h('span', { class: 'card__aside', text: sc.unit }),
    ]),
    h('div', { class: 'scheduler__metrics' }, sc.metrics.map(metric)),
    h('div', { class: 'scheduler__source', text: sc.source }),
  ]);
}

/**
 * Render the CPU performance screen.
 * @param {HTMLElement} container Shell content slot (emptied first).
 * @param {object} data CPU_PERFORMANCE_MOCK-shaped data.
 */
export function renderCpuPerformanceView(container, data) {
  container.replaceChildren(h('div', { class: 'perf' }, [
    resourceSidebar(data),
    h('div', { class: 'cpu' }, [
      cpuHeader(data.header),
      h('div', { class: 'cpu__row cpu__row--top' }, [gaugesCard(data), utilizationCard(data), topProcessesCard(data)]),
      h('div', { class: 'cpu__row cpu__row--mid' }, [summaryCard(data), specificationsCard(data)]),
      h('div', { class: 'cpu__row cpu__row--bottom' }, [breakdownCard(data), schedulerCard(data)]),
    ]),
  ]));
}
