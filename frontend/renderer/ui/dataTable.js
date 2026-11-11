/**
 * @file   dataTable.js
 * @brief  Presentational table: sticky header with sort arrows, rows rebuilt on every update, row
 *         selection. It owns no data and no sort logic: the caller passes ready-to-show rows and the
 *         sort state, and is told which column header was clicked.
 */
import { h } from './dom.js';

const SORT_ARROW_ASCENDING = '▲';
const SORT_ARROW_DESCENDING = '▼';

/**
 * @typedef {object} Column
 * @property {string} key Unique id of the column.
 * @property {string} label Header text.
 * @property {'left'|'right'} [align] Text alignment (default left).
 * @property {string} [width] CSS width of the column, e.g. "90px"; unset = share the remaining width.
 * @property {boolean} [sortable] Header click sorts (default true).
 * @property {(row: object) => string} [text] Cell text.
 * @property {(row: object) => (string|Node)} [render] Cell content when plain text is not enough.
 * @property {(row: object) => string} [className] Extra CSS class of the cell.
 */

/**
 * Create a table.
 * @param {object} options
 * @param {Column[]} options.columns
 * @param {(row: object) => string} options.rowKey Stable id of a row (keeps the selection across updates).
 * @param {(columnKey: string) => void} options.onSort Called when a sortable header is clicked.
 * @param {(row: object) => void} [options.onRowClick] Called when a row is clicked.
 * @param {(row: object) => string} [options.rowClass] Extra CSS class of a row.
 * @param {string} [options.emptyText] Text shown when there are no rows.
 * @returns {{element: HTMLElement, update: (state: {rows: object[], sortKey: string, sortDirection: string}) => void}}
 */
export function createDataTable({ columns, rowKey, onSort, onRowClick, rowClass, emptyText = 'No data' }) {
  let selectedKey = null;

  const headerCells = columns.map((column) => {
    const arrow = h('span', { class: 'table__sort' });
    const cell = h('th', {
      class: column.align === 'right' ? 'table__cell--right' : '',
      style: column.width ? { width: column.width } : {},
      on: column.sortable === false ? {} : { click: () => onSort(column.key) },
    }, [column.label, arrow]);
    if (column.sortable !== false) cell.classList.add('table__head--sortable');
    return { column, cell, arrow };
  });
  const body = h('tbody');
  const element = h('div', { class: 'table-wrap' }, [
    h('table', { class: 'table' }, [h('thead', {}, [h('tr', {}, headerCells.map((part) => part.cell))]), body]),
  ]);

  /**
   * Build one row.
   * @param {object} row
   * @returns {HTMLElement}
   */
  function buildRow(row) {
    const key = rowKey(row);
    const classes = [rowClass?.(row) ?? '', key === selectedKey ? 'is-selected' : ''].filter(Boolean).join(' ');
    const tr = h('tr', { class: classes }, columns.map((column) => {
      const content = column.render ? column.render(row) : column.text(row);
      const extra = column.className?.(row) ?? '';
      return h('td', { class: `${column.align === 'right' ? 'table__cell--right' : ''} ${extra}`.trim() }, [content]);
    }));
    tr.addEventListener('click', () => {
      selectedKey = key;
      for (const other of body.querySelectorAll('.is-selected')) other.classList.remove('is-selected');
      tr.classList.add('is-selected');
      onRowClick?.(row);
    });
    return tr;
  }

  return {
    element,
    update({ rows, sortKey, sortDirection }) {
      for (const { column, cell, arrow } of headerCells) {
        const active = column.key === sortKey;
        cell.classList.toggle('table__head--active', active);
        arrow.textContent = active ? (sortDirection === 'asc' ? SORT_ARROW_ASCENDING : SORT_ARROW_DESCENDING) : '';
      }
      if (rows.length === 0) {
        body.replaceChildren(h('tr', { class: 'table__empty' }, [
          h('td', { colspan: columns.length, text: emptyText }),
        ]));
        return;
      }
      body.replaceChildren(...rows.map(buildRow));
    },
  };
}

/**
 * Compare two column values: numbers numerically, everything else as text with numeric awareness.
 * @param {number|string} a
 * @param {number|string} b
 * @returns {number} Negative, zero or positive, like Array.prototype.sort expects.
 */
export function compareValues(a, b) {
  if (typeof a === 'number' && typeof b === 'number') return a - b;
  return String(a).localeCompare(String(b), undefined, { numeric: true, sensitivity: 'base' });
}
