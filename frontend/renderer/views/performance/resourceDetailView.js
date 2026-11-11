/**
 * @file   resourceDetailView.js
 * @brief  Generic detail panel of a monitored resource (Memory, Disk, Network, GPU): title, one or
 *         more history charts, a card of figures, a card of facts and an optional table. What it shows
 *         comes entirely from the model (see resourceDetailModels.js).
 *
 * The frame (cards, titles, info icons) is built on the first update(), when the number of charts and
 * the presence of the table are known; later updates only replace the changing content, so open
 * tooltips survive the once-per-second refresh. One instance is used for one kind of resource.
 */
import { h } from '../../ui/dom.js';
import { infoIcon } from '../../ui/tooltip.js';
import { plot } from '../../ui/charts/plot.js';
import { DETAIL_TEXTS } from './resourceDetailTexts.js';

/** Drawing size of a chart; its width stretches to the card (fluid plot). */
const CHART_PLOT = { width: 1100, height: 150, grid: { columns: 20, rows: 5 }, fluid: true };

/**
 * Chart legend: coloured swatch + name per entry.
 * @param {Array<{name: string, color: string}>} entries
 * @returns {HTMLElement}
 */
function legend(entries) {
  return h('div', { class: 'legend' }, entries.map(({ name, color }) => {
    const swatch = h('span', { class: 'legend__swatch' });
    swatch.style.background = color;
    return h('span', { class: 'legend__item' }, [swatch, name]);
  }));
}

/**
 * One chart card.
 * @param {object} chart Chart model (title and tooltip are fixed for the life of the card).
 * @returns {{element: HTMLElement, update: (chart: object) => void}}
 */
function chartCard(chart) {
  const topRight = h('span', { class: 'chart__axis' });
  const plotSlot = h('div', { class: 'slot' });
  const legendSlot = h('div', { class: 'slot' });
  const element = h('section', { class: 'card detail__card' }, [
    h('div', { class: 'chart__top' }, [
      h('div', { class: 'card__title card__title--small' }, [chart.title, infoIcon(chart.tooltip)]),
      topRight,
    ]),
    plotSlot,
    h('div', { class: 'chart__bottom' }, [
      h('span', { class: 'chart__axis', text: DETAIL_TEXTS.xStart }),
      legendSlot,
      h('span', { class: 'chart__axis', text: DETAIL_TEXTS.xEnd }),
    ]),
  ]);
  return {
    element,
    update(next) {
      topRight.textContent = next.topRight;
      plotSlot.replaceChildren(plot({ ...CHART_PLOT, series: next.series, className: 'chart__plot' }));
      legendSlot.replaceChildren(legend(next.legend));
    },
  };
}

/**
 * Table card (e.g. top memory processes).
 * @param {object} table Table model (title, tooltip, columns are fixed for the life of the card).
 * @returns {{element: HTMLElement, update: (table: object) => void}}
 */
function tableCard(table) {
  const body = h('tbody');
  const element = h('section', { class: 'card detail__card' }, [
    h('div', { class: 'card__title card__title--table' }, [table.title, infoIcon(table.tooltip)]),
    h('table', { class: 'detail-table' }, [
      h('thead', {}, [h('tr', {}, table.columns.map((column) =>
        h('th', { class: column.align === 'right' ? 'detail-table__cell--right' : '', text: column.label })))]),
      body,
    ]),
  ]);
  return {
    element,
    update(next) {
      body.replaceChildren(...next.rows.map((row) => h('tr', { class: row.self ? 'detail-table__row--self' : '' },
        row.cells.map((cell, index) => h('td', {
          class: next.columns[index].align === 'right' ? 'detail-table__cell--right' : '',
          text: cell,
        })))));
    },
  };
}

/**
 * Create the panel.
 * @returns {{element: HTMLElement, update: (model: object) => void}}
 */
export function createResourceDetailView() {
  const title = h('h1', { class: 'detail__title' });
  const subtitle = h('div', { class: 'detail__subtitle' });
  const charts = h('div', { class: 'detail__charts' });
  const stats = h('div', { class: 'detail__stats' });
  const infoRows = h('div', { class: 'detail__info' });
  const row = h('div', { class: 'detail__row' });
  const tableHost = h('div', { class: 'detail__table' });
  const element = h('div', { class: 'detail' }, [
    h('div', { class: 'detail__header' }, [title, subtitle]),
    charts,
    row,
    tableHost,
  ]);

  let chartParts = null;
  let tablePart = null;

  /**
   * Build the cards from the first model.
   * @param {object} model
   */
  function buildFrame(model) {
    chartParts = model.charts.map(chartCard);
    charts.replaceChildren(...chartParts.map((part) => part.element));
    row.replaceChildren(
      h('section', { class: 'card detail__card detail__stats-card' }, [
        stats,
        infoIcon(model.statsTooltip, 'detail__stats-info'),
      ]),
      h('section', { class: 'card detail__card detail__info-card' }, [
        h('div', { class: 'card__title' }, [model.info.title, infoIcon(model.info.tooltip)]),
        infoRows,
      ]),
    );
    tablePart = model.table ? tableCard(model.table) : null;
    tableHost.replaceChildren(...(tablePart ? [tablePart.element] : []));
  }

  return {
    element,
    update(model) {
      if (chartParts === null) buildFrame(model);
      title.textContent = model.title;
      subtitle.textContent = model.subtitle;
      chartParts.forEach((part, index) => part.update(model.charts[index]));
      stats.replaceChildren(...model.stats.map(({ label, value }) => h('div', { class: 'metric' }, [
        h('div', { class: 'metric__label', text: label }),
        h('div', { class: 'metric__value', text: value }),
      ])));
      infoRows.replaceChildren(...model.info.rows.map(([label, value]) => h('div', { class: 'spec__row' }, [
        h('span', { class: 'spec__label', text: label }),
        h('span', { class: 'spec__value', text: value }),
      ])));
      tablePart?.update(model.table);
    },
  };
}
