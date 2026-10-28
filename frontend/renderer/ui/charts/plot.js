/**
 * @file   plot.js
 * @brief  SVG time-series plot: optional grid, filled area and line series.
 *         Used by the sidebar sparklines, the % Utilization chart and the User vs Kernel trend.
 */
import { s } from '../dom.js';

/** Full scale of every series: values are percentages. */
const MAX_VALUE = 100;

/**
 * Convert values (0..100, oldest first) to SVG points spread over the plot width.
 * @param {number[]} values Samples.
 * @param {number} width Plot width (px).
 * @param {number} height Plot height (px).
 * @param {number} inset Margin (px) kept free on every side, e.g. inside a sparkline border.
 * @returns {Array<[number, number]>}
 */
function toPoints(values, width, height, inset) {
  const innerWidth = width - 2 * inset;
  const innerHeight = height - 2 * inset;
  const step = innerWidth / (values.length - 1);
  return values.map((v, i) => [inset + i * step, inset + innerHeight - (v / MAX_VALUE) * innerHeight]);
}

/**
 * @param {Array<[number, number]>} points
 * @returns {string} SVG "points" attribute.
 */
function pointsAttr(points) {
  return points.map(([x, y]) => `${x.toFixed(2)},${y.toFixed(2)}`).join(' ');
}

/**
 * Grid lines.
 * @param {number} width Plot width.
 * @param {number} height Plot height.
 * @param {{columns: number, rows: number}} grid Number of cells; rows 0 = no horizontal lines.
 * @returns {SVGElement[]}
 */
function gridLines(width, height, { columns, rows }) {
  const lines = [];
  for (let i = 1; i < columns; i++) {
    const x = (i * width) / columns;
    lines.push(s('line', { class: 'plot__grid', x1: x, y1: 0, x2: x, y2: height }));
  }
  for (let i = 1; i < rows; i++) {
    const y = (i * height) / rows;
    lines.push(s('line', { class: 'plot__grid', x1: 0, y1: y, x2: width, y2: y }));
  }
  return lines;
}

/**
 * Build a plot.
 * @param {object} options
 * @param {number} options.width Outer width including the 1px border.
 * @param {number} options.height Outer height including the 1px border.
 * @param {{columns: number, rows: number}} [options.grid] Grid cells; omitted = no grid.
 * @param {Array<{values: number[], color: string, fill?: boolean}>} options.series Drawn in order
 *        (later series on top). fill draws a translucent area under the line.
 * @param {string} [options.className] Extra class on the <svg>.
 * @param {number} [options.inset] Margin (px) between the series and the plot edges.
 * @returns {SVGElement}
 */
export function plot({ width, height, grid, series, className = '', inset = 0 }) {
  const children = grid ? gridLines(width, height, grid) : [];
  const bottom = height - inset;
  for (const { values, color, fill } of series) {
    const points = toPoints(values, width, height, inset);
    if (fill) {
      children.push(s('polygon', {
        class: 'plot__area',
        points: pointsAttr([[points[0][0], bottom], ...points, [points[points.length - 1][0], bottom]]),
        // Colours go through style, not attributes: SVG attributes do not resolve var(--x).
        style: { fill: color },
      }));
    }
    children.push(s('polyline', { class: 'plot__line', points: pointsAttr(points), style: { stroke: color } }));
  }
  // The border is drawn last on top so lines touching the edges do not cover it.
  children.push(s('rect', { class: 'plot__border', x: 0.5, y: 0.5, width: width - 1, height: height - 1 }));
  return s('svg', {
    class: `plot ${className}`.trim(), width, height, viewBox: `0 0 ${width} ${height}`,
  }, children);
}
