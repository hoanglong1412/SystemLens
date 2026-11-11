/**
 * @file   tableScreen.js
 * @brief  A screen made of a title row, an optional filter box and select filters, an optional
 *         "not available" banner and a sortable table. Keeps the sort, filter and selection state, so a
 *         data refresh only calls update() with new rows.
 */
import { h } from './dom.js';
import { compareValues, createDataTable } from './dataTable.js';
import { createSearchBox } from './searchBox.js';

const ASCENDING = 'asc';
const DESCENDING = 'desc';

/**
 * @typedef {object} TableScreenOptions
 * @property {string} title Screen / section title.
 * @property {import('./dataTable.js').Column[]} columns Columns; each sortable column needs sortValue(row).
 * @property {{key: string, direction: 'asc'|'desc'}} defaultSort Initial order.
 * @property {(row: object) => string} rowKey Stable id of a row.
 * @property {(row: object) => string} [searchText] Text the filter box matches; omit for no filter box.
 * @property {string} [searchPlaceholder]
 * @property {Array<{label: string, options: Array<{value: string, label: string}>, predicate: (row: object, value: string) => boolean}>} [filters]
 *           Select boxes; the first option is the "no filtering" choice.
 * @property {(shown: object[], all: object[]) => string} [summary] Text right of the toolbar, e.g. "12 of 40".
 * @property {(row: object) => string} [rowClass] Extra CSS class of a row.
 * @property {(row: object, api: {rerender: () => void}) => void} [onRowClick]
 * @property {(rows: object[], view: object) => object[]} [transform] Replaces the default filter + sort
 *           (used by the grouped process list). view = {sortKey, sortDirection, search, filterValues}.
 * @property {string} [emptyText]
 * @property {boolean} [compact] Table height follows its rows (up to a limit) instead of filling the screen.
 */

/**
 * Create a table screen.
 * @param {TableScreenOptions} options
 * @returns {{element: HTMLElement, update: (data: {rows: object[], unavailableReason?: string|null, note?: string|null}) => void}}
 */
export function createTableScreen(options) {
  const view = {
    sortKey: options.defaultSort.key,
    sortDirection: options.defaultSort.direction,
    search: '',
    filterValues: (options.filters ?? []).map((filter) => filter.options[0].value),
  };
  let rows = [];

  const table = createDataTable({
    columns: options.columns,
    rowKey: options.rowKey,
    rowClass: options.rowClass,
    emptyText: options.emptyText,
    onSort: (key) => {
      if (view.sortKey === key) {
        view.sortDirection = view.sortDirection === ASCENDING ? DESCENDING : ASCENDING;
      } else {
        view.sortKey = key;
        // Right-aligned columns are numbers: the biggest first is what people look for.
        view.sortDirection = options.columns.find((c) => c.key === key)?.align === 'right' ? DESCENDING : ASCENDING;
      }
      render();
    },
    onRowClick: (row) => options.onRowClick?.(row, { rerender: render }),
  });

  const note = h('span', { class: 'toolbar__note' });
  const summary = h('span', { class: 'toolbar__summary' });
  const banner = h('div', { class: 'banner is-hidden', role: 'status' });
  const toolbarItems = [h('h2', { class: 'toolbar__title', text: options.title })];
  if (options.searchText) {
    toolbarItems.push(createSearchBox({
      placeholder: options.searchPlaceholder ?? 'Filter',
      onInput: (text) => {
        view.search = text.trim().toLowerCase();
        render();
      },
    }));
  }
  (options.filters ?? []).forEach((filter, index) => {
    const select = h('select', { class: 'select', 'aria-label': filter.label }, filter.options.map((option) =>
      h('option', { value: option.value, text: option.label })));
    select.addEventListener('change', () => {
      view.filterValues[index] = select.value;
      render();
    });
    toolbarItems.push(select);
  });
  toolbarItems.push(note, summary);

  const element = h('section', { class: options.compact ? 'screen screen--compact' : 'screen' }, [
    h('div', { class: 'toolbar' }, toolbarItems),
    banner,
    table.element,
  ]);

  /**
   * Default arrangement: filter by text and select boxes, then sort by the active column.
   * @returns {object[]}
   */
  function arrange() {
    let shown = rows;
    if (options.searchText && view.search) {
      shown = shown.filter((row) => options.searchText(row).toLowerCase().includes(view.search));
    }
    (options.filters ?? []).forEach((filter, index) => {
      shown = shown.filter((row) => filter.predicate(row, view.filterValues[index]));
    });
    const column = options.columns.find((c) => c.key === view.sortKey);
    const direction = view.sortDirection === ASCENDING ? 1 : -1;
    // Array.prototype.sort is stable, so equal rows keep the order the backend sent them in.
    return [...shown].sort((a, b) => direction * compareValues(column.sortValue(a), column.sortValue(b)));
  }

  /** Recompute the shown rows and redraw the table. */
  function render() {
    const shown = options.transform ? options.transform(rows, view) : arrange();
    table.update({ rows: shown, sortKey: view.sortKey, sortDirection: view.sortDirection });
    summary.textContent = options.summary ? options.summary(shown, rows) : '';
  }

  render();
  return {
    element,
    update({ rows: nextRows, unavailableReason = null, note: noteText = null }) {
      rows = nextRows;
      banner.classList.toggle('is-hidden', unavailableReason === null);
      banner.textContent = unavailableReason === null ? '' : `Not available on this system: ${unavailableReason}`;
      note.textContent = noteText ?? '';
      render();
    },
  };
}
