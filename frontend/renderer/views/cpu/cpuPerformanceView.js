/**
 * @file   cpuPerformanceView.js
 * @brief  Performance > CPU panel: gauges, utilization chart, top processes, summary, specifications,
 *         CPU time breakdown and scheduler activity (sub-tab "Overview"), or one card per logical
 *         processor (sub-tab "Per-core"). The resource sidebar lives in views/performance.
 *
 * The cards are built once; update() replaces only the content that changes (texts, plots, table
 * rows), so the info icons and their open tooltips survive the once-per-second refresh.
 * Display data comes from cpuPerformanceModel.js; static texts from cpuPerformanceTexts.js.
 */
import { dotted, h } from '../../ui/dom.js';
import { infoIcon } from '../../ui/tooltip.js';
import { plot } from '../../ui/charts/plot.js';
import { ledGauge } from '../../ui/charts/ledGauge.js';
import { CPU_TEXTS } from './cpuPerformanceTexts.js';

/** Plot sizes from the design (outer size, border included). */
const UTILIZATION_PLOT = { width: 588, height: 228, grid: { columns: 20, rows: 10 } };
const TREND_PLOT = { width: 584, height: 52, grid: { columns: 20, rows: 0 } };
/** Drawing size of one per-core plot; it stretches to the card width. */
const CORE_PLOT = { width: 240, height: 56, grid: { columns: 12, rows: 4 }, fluid: true };

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
 * A part of the screen: its element and the function that shows new data in it.
 * @typedef {{element: HTMLElement, update: (data: object) => void}} Part
 */

/**
 * CPU title, model, topology and Overview / Per-core sub-tabs.
 * @param {(name: string) => void} onSubTabChange Called when a sub-tab is clicked.
 * @returns {Part & {setActiveSubTab: (name: string) => void}}
 */
function cpuHeader(onSubTabChange) {
  const model = h('div', { class: 'cpu-header__model' });
  const topology = h('div', { class: 'cpu-header__topology' });
  const buttons = CPU_TEXTS.subTabs.map((name) => h('button', {
    type: 'button', class: 'subtab', text: name, on: { click: () => onSubTabChange(name) },
  }));
  const element = h('div', { class: 'cpu-header' }, [
    h('h1', { class: 'cpu-header__title', text: CPU_TEXTS.title }),
    model,
    topology,
    h('div', { class: 'subtabs', role: 'tablist' }, buttons),
  ]);
  return {
    element,
    update({ header }) {
      model.textContent = header.model;
      topology.replaceChildren(...dotted(header.topology));
    },
    setActiveSubTab(name) {
      buttons.forEach((button) => button.classList.toggle('subtab--active', button.textContent === name));
    },
  };
}

/**
 * Live gauges card.
 * @returns {Part}
 */
function gaugesCard() {
  const gauges = h('div', { class: 'gauges' });
  return {
    element: card('gauges', [
      cardTitle(CPU_TEXTS.gauges.title, CPU_TEXTS.tooltips.gauges, 'card__title--small'),
      gauges,
    ]),
    update(data) {
      gauges.replaceChildren(...data.gauges.map(ledGauge));
    },
  };
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
 * @returns {Part}
 */
function utilizationCard() {
  const texts = CPU_TEXTS.utilization;
  const plotSlot = h('div', { class: 'slot' });
  const legendSlot = h('div', { class: 'slot' });
  return {
    element: card('utilization', [
      h('div', { class: 'chart__top' }, [
        cardTitle(texts.title, CPU_TEXTS.tooltips.utilization, 'card__title--small'),
        h('span', { class: 'chart__axis', text: texts.max }),
      ]),
      plotSlot,
      h('div', { class: 'chart__bottom' }, [
        h('span', { class: 'chart__axis', text: texts.xStart }),
        legendSlot,
        h('span', { class: 'chart__axis', text: texts.xEnd }),
      ]),
    ]),
    update({ utilization }) {
      // The usage area is first in the list so the clock / temperature lines stay visible over it.
      plotSlot.replaceChildren(plot({ ...UTILIZATION_PLOT, series: utilization.series, className: 'chart__plot' }));
      legendSlot.replaceChildren(legend(utilization.legend));
    },
  };
}

/**
 * Top CPU processes table card.
 * @returns {Part}
 */
function topProcessesCard() {
  const title = h('span', { class: 'slot' });
  const body = h('tbody');
  return {
    element: card('processes', [
      h('div', { class: 'card__title card__title--table' }, [title, infoIcon(CPU_TEXTS.tooltips.topProcesses)]),
      h('table', { class: 'proc' }, [
        h('thead', {}, [h('tr', {}, CPU_TEXTS.topProcesses.columns.map((c) => h('th', { text: c })))]),
        body,
      ]),
    ]),
    update({ topProcesses }) {
      title.textContent = `» ${topProcesses.title}`;
      body.replaceChildren(...topProcesses.rows.map((row) => {
        const cpu = h('td', { class: 'proc__cpu', text: row.cpu });
        // backgroundColor, not the background shorthand: the shorthand would reset background-clip.
        cpu.style.backgroundColor = row.cpuShade;
        return h('tr', {}, [
          h('td', { class: 'proc__pid', text: String(row.pid) }),
          h('td', { class: row.self ? 'proc__name proc__name--self' : 'proc__name', text: row.name }),
          cpu,
          h('td', { class: 'proc__mem', text: row.memory }),
        ]);
      }));
    },
  };
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
 * @returns {Part}
 */
function summaryCard() {
  const metrics = h('div', { class: 'summary__metrics' });
  const uptime = h('div', { class: 'summary__uptime' });
  return {
    element: card('summary', [
      metrics,
      h('div', { class: 'summary__divider' }),
      uptime,
      infoIcon(CPU_TEXTS.tooltips.summary, 'summary__info'),
    ]),
    update({ summary }) {
      metrics.replaceChildren(...summary.metrics.map(metric));
      uptime.replaceChildren(
        h('div', { class: 'metric__label', text: summary.uptime.label }),
        h('div', { class: 'summary__uptime-value', text: summary.uptime.value }),
        h('div', { class: 'metric__label summary__started-label', text: summary.uptime.startedLabel }),
        h('div', { class: 'summary__started', text: summary.uptime.started }),
      );
    },
  };
}

/**
 * Specifications card: two columns of label/value rows.
 * @returns {Part}
 */
function specificationsCard() {
  const spec = h('div', { class: 'spec' });
  return {
    element: card('spec', [
      cardTitle(CPU_TEXTS.specifications.title, CPU_TEXTS.tooltips.specifications),
      spec,
    ]),
    update({ specifications }) {
      spec.replaceChildren(...specifications.columns.map((rows) =>
        h('div', { class: 'spec__column' }, rows.map(([label, value]) =>
          h('div', { class: 'spec__row' }, [
            h('span', { class: 'spec__label', text: label }),
            h('span', { class: 'spec__value', text: value }),
          ])))));
    },
  };
}

/**
 * CPU time breakdown card: stacked bar, legend with percentages and User vs Kernel trend.
 * @returns {Part}
 */
function breakdownCard() {
  const texts = CPU_TEXTS.breakdown;
  const bar = h('div', { class: 'breakdown__bar' });
  const legendRow = h('div', { class: 'breakdown__legend' });
  const trendSlot = h('div', { class: 'slot' });
  return {
    element: card('breakdown', [
      h('div', { class: 'card__head' }, [
        cardTitle(texts.title, CPU_TEXTS.tooltips.breakdown),
        h('span', { class: 'card__aside' }, dotted(texts.scope)),
      ]),
      bar,
      legendRow,
      h('div', { class: 'breakdown__caption' }, dotted(texts.trendCaption)),
      trendSlot,
    ]),
    update({ breakdown }) {
      bar.replaceChildren(...breakdown.parts.map(({ color, width }) => {
        const seg = h('span', { class: width === null ? 'breakdown__seg breakdown__seg--rest' : 'breakdown__seg' });
        seg.style.background = color;
        if (width !== null) seg.style.width = `${width}px`;
        return seg;
      }));
      legendRow.replaceChildren(...breakdown.parts.map(({ name, percent, color }) => {
        const swatch = h('span', { class: 'breakdown__swatch' });
        swatch.style.background = color;
        return h('div', { class: 'breakdown__item' }, [
          swatch,
          h('div', {}, [
            h('div', { class: 'breakdown__name', text: name }),
            h('div', { class: 'breakdown__percent', text: percent }),
          ]),
        ]);
      }));
      trendSlot.replaceChildren(plot({ ...TREND_PLOT, series: breakdown.trend, className: 'chart__plot chart__plot--trend' }));
    },
  };
}

/**
 * Scheduler activity card.
 * @returns {Part}
 */
function schedulerCard() {
  const texts = CPU_TEXTS.scheduler;
  const metrics = h('div', { class: 'scheduler__metrics' });
  const source = h('div', { class: 'scheduler__source' });
  return {
    element: card('scheduler', [
      h('div', { class: 'card__head' }, [
        cardTitle(texts.title, CPU_TEXTS.tooltips.scheduler),
        h('span', { class: 'card__aside', text: texts.unit }),
      ]),
      metrics,
      source,
    ]),
    update({ scheduler }) {
      metrics.replaceChildren(...scheduler.metrics.map(metric));
      source.textContent = scheduler.source;
    },
  };
}

/**
 * Per-core grid: one card per logical processor with its current usage and 60-second plot.
 * @returns {Part}
 */
function coresGrid() {
  const element = h('div', { class: 'cores' });
  return {
    element,
    update({ cores }) {
      element.replaceChildren(...cores.map((core) => h('div', { class: 'card core' }, [
        h('div', { class: 'core__head' }, [
          h('span', { class: 'core__label', text: core.label }),
          h('span', { class: 'core__value', text: core.value }),
        ]),
        plot({ ...CORE_PLOT, series: [{ values: core.series, color: 'var(--green)', fill: true }], className: 'chart__plot' }),
      ])));
    },
  };
}

/**
 * Build the CPU panel.
 * @param {object} data Initial display data (see cpuPerformanceModel.js, toCpuViewData).
 * @returns {{element: HTMLElement, update: (data: object) => void}} update: show new display data.
 */
export function createCpuPerformanceView(data) {
  const header = cpuHeader(showSubTab);
  const top = [gaugesCard(), utilizationCard(), topProcessesCard()];
  const middle = [summaryCard(), specificationsCard()];
  const bottom = [breakdownCard(), schedulerCard()];
  const cores = coresGrid();
  const elements = (parts) => parts.map((part) => part.element);

  const overview = h('div', { class: 'cpu__overview' }, [
    h('div', { class: 'cpu__row cpu__row--top' }, elements(top)),
    h('div', { class: 'cpu__row cpu__row--mid' }, elements(middle)),
    h('div', { class: 'cpu__row cpu__row--bottom' }, elements(bottom)),
  ]);
  const element = h('div', { class: 'cpu' }, [header.element, overview, cores.element]);

  /**
   * Show the Overview or the Per-core content.
   * @param {string} name Sub-tab name from CPU_TEXTS.subTabs.
   */
  function showSubTab(name) {
    const perCore = name === CPU_TEXTS.subTabs[1];
    overview.classList.toggle('is-hidden', perCore);
    cores.element.classList.toggle('is-hidden', !perCore);
    header.setActiveSubTab(name);
  }

  showSubTab(CPU_TEXTS.activeSubTab);
  const parts = [header, ...top, ...middle, ...bottom, cores];
  const update = (next) => parts.forEach((part) => part.update(next));
  update(data);
  return { element, update };
}
