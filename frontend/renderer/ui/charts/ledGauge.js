/**
 * @file   ledGauge.js
 * @brief  Vertical LED bar gauge (label, segmented bar, value) used in the "Live gauges" card.
 */
import { h, s } from '../dom.js';

const BAR_WIDTH = 28;
const BAR_HEIGHT = 206;
const SEGMENT_COUNT = 40;
/** Inner area of the bar (inside the 1px border). */
const INNER_TOP = 1;
const INNER_HEIGHT = BAR_HEIGHT - 2;
/** Horizontal inset of the segments inside the border. */
const SEGMENT_INSET_X = 3;
/** Lit share of each segment slot; the rest is the dark gap between segments. */
const SEGMENT_FILL_RATIO = 0.68;

/**
 * Build one gauge.
 * @param {object} gauge
 * @param {string} gauge.label Label above the bar ("Usage").
 * @param {string} gauge.value Text below the bar ("5%").
 * @param {number} gauge.percent 0..100: share of lit segments, counted from the bottom.
 * @param {string} gauge.color CSS colour of the gauge.
 * @returns {HTMLElement}
 */
export function ledGauge({ label, value, percent, color }) {
  const slot = INNER_HEIGHT / SEGMENT_COUNT;
  const litCount = Math.round((percent / 100) * SEGMENT_COUNT);
  const segments = [];
  for (let i = 0; i < SEGMENT_COUNT; i++) {
    // i = 0 is the top segment; the lit ones are the last litCount.
    const lit = i >= SEGMENT_COUNT - litCount;
    segments.push(s('rect', {
      class: lit ? 'led__seg led__seg--on' : 'led__seg',
      x: SEGMENT_INSET_X,
      y: INNER_TOP + i * slot + slot * (1 - SEGMENT_FILL_RATIO) / 2,
      width: BAR_WIDTH - 2 * SEGMENT_INSET_X,
      height: slot * SEGMENT_FILL_RATIO,
    }));
  }

  const root = h('div', { class: 'led' }, [
    h('div', { class: 'led__label', text: label }),
    s('svg', { class: 'led__bar', width: BAR_WIDTH, height: BAR_HEIGHT, viewBox: `0 0 ${BAR_WIDTH} ${BAR_HEIGHT}` }, [
      s('rect', { class: 'led__frame', x: 0.5, y: 0.5, width: BAR_WIDTH - 1, height: BAR_HEIGHT - 1 }),
      ...segments,
    ]),
    h('div', { class: 'led__value', text: value }),
  ]);
  root.style.color = color;
  return root;
}
