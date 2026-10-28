/**
 * @file   icons.js
 * @brief  Small SVG icons drawn from the Figma design (app logo, monitor, sun, window controls).
 */
import { s } from './dom.js';

/**
 * App logo: blue rounded square with a white trend line.
 * @returns {SVGElement} 16x16 icon.
 */
export function appLogoIcon() {
  return s('svg', { class: 'icon-app', width: 16, height: 16, viewBox: '0 0 16 16' }, [
    s('rect', { width: 16, height: 16, rx: 3, style: { fill: 'var(--accent)' } }),
    s('polyline', {
      points: '3.5,11 6.5,7 9,9 12.5,4.5', fill: 'none', stroke: '#ffffff',
      'stroke-width': 1.6, 'stroke-linecap': 'round', 'stroke-linejoin': 'round',
    }),
  ]);
}

/**
 * Monitor outline used as the machine icon in the header.
 * @returns {SVGElement} 28x28 icon.
 */
export function monitorIcon() {
  return s('svg', { width: 28, height: 28, viewBox: '0 0 28 28', fill: 'none', 'stroke-width': 2, style: { stroke: 'var(--accent)' } }, [
    s('rect', { x: 2, y: 4, width: 24, height: 16, rx: 2 }),
    s('line', { x1: 10, y1: 24, x2: 18, y2: 24, 'stroke-linecap': 'round' }),
  ]);
}

/**
 * Sun used by the theme toggle button.
 * @returns {SVGElement} 14x14 icon.
 */
export function sunIcon() {
  const rays = [];
  const RAY_COUNT = 8;
  for (let i = 0; i < RAY_COUNT; i++) {
    const angle = (i * Math.PI * 2) / RAY_COUNT;
    rays.push(s('line', {
      x1: 7 + Math.cos(angle) * 4.6, y1: 7 + Math.sin(angle) * 4.6,
      x2: 7 + Math.cos(angle) * 6.2, y2: 7 + Math.sin(angle) * 6.2,
    }));
  }
  return s('svg', { width: 14, height: 14, viewBox: '0 0 14 14', style: { stroke: 'var(--yellow)' }, 'stroke-width': 1.3, 'stroke-linecap': 'round' }, [
    s('circle', { cx: 7, cy: 7, r: 2.8, style: { fill: 'var(--yellow)', stroke: 'none' } }),
    ...rays,
  ]);
}

/**
 * Window control glyph (minimize / maximize / close), 12x12.
 * @param {'minimize'|'maximize'|'close'} kind
 * @returns {SVGElement}
 */
export function windowControlIcon(kind) {
  const shapes = {
    minimize: [s('line', { x1: 1, y1: 6.5, x2: 11, y2: 6.5 })],
    maximize: [s('rect', { x: 1.5, y: 1.5, width: 9, height: 9 })],
    close: [s('line', { x1: 1, y1: 1, x2: 11, y2: 11 }), s('line', { x1: 11, y1: 1, x2: 1, y2: 11 })],
  };
  return s('svg', { width: 12, height: 12, viewBox: '0 0 12 12', fill: 'none', 'stroke-width': 1, style: { stroke: 'var(--text-muted)' } }, shapes[kind]);
}
