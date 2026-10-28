/**
 * @file   tooltip.js
 * @brief  Info icon ("i" in a circle) with a hover tooltip, as in the Figma tooltip layer.
 *
 * One shared tooltip element lives at the end of <body>; each icon only stores its text and
 * placement, so any view can add icons without managing popups itself.
 */
import { h, s } from './dom.js';

/** Gap between the icon and the tooltip box (px). */
const TOOLTIP_OFFSET = 8;

/**
 * Where the tooltip opens relative to its icon:
 *  - bottom-start: below, left edges aligned (default)
 *  - bottom-end:   below, right edges aligned (icons near the right edge of a card)
 *  - top-start:    above, left edges aligned (icons in the status bar)
 */
export const Placement = Object.freeze({
  BOTTOM_START: 'bottom-start',
  BOTTOM_END: 'bottom-end',
  TOP_START: 'top-start',
});

let tooltipEl = null;

/**
 * Lazily create the shared tooltip element.
 * @returns {HTMLElement}
 */
function getTooltipElement() {
  if (!tooltipEl) {
    tooltipEl = h('div', { class: 'tooltip', role: 'tooltip' }, [
      h('div', { class: 'tooltip__title' }),
      h('div', { class: 'tooltip__body' }),
    ]);
    document.body.append(tooltipEl);
  }
  return tooltipEl;
}

/**
 * Show the shared tooltip next to an icon.
 * @param {Element} icon Hovered icon.
 * @param {{title: string, body: string, placement: string}} tip
 */
function showTooltip(icon, { title, body, placement }) {
  const el = getTooltipElement();
  el.querySelector('.tooltip__title').textContent = title;
  el.querySelector('.tooltip__body').textContent = body;
  el.classList.add('tooltip--visible');

  // Measure after the text is set: the height depends on how the body wraps.
  const anchor = icon.getBoundingClientRect();
  const box = el.getBoundingClientRect();
  const left = placement === Placement.BOTTOM_END ? anchor.right - box.width : anchor.left;
  const top = placement === Placement.TOP_START
    ? anchor.top - TOOLTIP_OFFSET - box.height
    : anchor.bottom + TOOLTIP_OFFSET;
  el.style.left = `${left}px`;
  el.style.top = `${top}px`;
}

/** Hide the shared tooltip. */
function hideTooltip() {
  getTooltipElement().classList.remove('tooltip--visible');
}

/**
 * Create a 16x16 info icon that shows a tooltip on hover.
 * @param {object} tip
 * @param {string} tip.title Bold first line.
 * @param {string} tip.body Description.
 * @param {string} [tip.placement] One of Placement.
 * @param {string} [extraClass] Extra CSS class for positioning by the caller.
 * @returns {HTMLElement}
 */
export function infoIcon({ title, body, placement = Placement.BOTTOM_START }, extraClass = '') {
  const icon = h('span', {
    class: `info-icon ${extraClass}`.trim(),
    'aria-label': title,
    tabindex: 0,
    on: {
      mouseenter: () => showTooltip(icon, { title, body, placement }),
      mouseleave: hideTooltip,
      focus: () => showTooltip(icon, { title, body, placement }),
      blur: hideTooltip,
    },
  }, [
    s('svg', { width: 16, height: 16, viewBox: '0 0 16 16' }, [
      s('circle', { class: 'info-icon__ring', cx: 8, cy: 8, r: 7 }),
      s('rect', { class: 'info-icon__glyph', x: 7.3, y: 4, width: 1.4, height: 1.4, rx: 0.7 }),
      s('rect', { class: 'info-icon__glyph', x: 7.35, y: 6.6, width: 1.3, height: 5.2, rx: 0.4 }),
    ]),
  ]);
  return icon;
}
