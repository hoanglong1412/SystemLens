/**
 * @file   resourceSidebar.js
 * @brief  Sidebar of the Performance screen: one selectable row per monitored resource with its sparkline.
 */
import { dotted, h } from '../../ui/dom.js';
import { infoIcon } from '../../ui/tooltip.js';
import { plot } from '../../ui/charts/plot.js';

/** Sparkline size from the design (outer size, border included). */
const SPARKLINE = { width: 72, height: 52, inset: 1 };

/**
 * Build the sidebar.
 * @param {object} options
 * @param {{title: string, body: string}} options.tooltip Tooltip of the info icon.
 * @param {(id: string) => void} options.onSelect Called with the resource id when a row is clicked or
 *        activated with the keyboard.
 * @returns {{element: HTMLElement, update: (rows: object[]) => void}}
 */
export function createResourceSidebar({ tooltip, onSelect }) {
  const rows = h('div', { class: 'slot' });
  const element = h('aside', { class: 'resources' }, [infoIcon(tooltip, 'resources__info'), rows]);
  return {
    element,
    update(resourceRows) {
      rows.replaceChildren(...resourceRows.map((res) => {
        const spark = plot({ ...SPARKLINE, series: [{ values: res.series, color: res.color, fill: res.fill }], className: 'sparkline' });
        spark.style.color = res.color;
        return h('div', {
          class: res.selected ? 'resource resource--selected' : 'resource',
          role: 'button',
          tabindex: 0,
          'aria-pressed': String(res.selected),
          on: {
            click: () => onSelect(res.id),
            keydown: (event) => {
              if (event.key === 'Enter' || event.key === ' ') {
                event.preventDefault();
                onSelect(res.id);
              }
            },
          },
        }, [
          spark,
          h('div', { class: 'resource__text' }, [
            h('div', { class: 'resource__name', text: res.name }),
            h('div', { class: 'resource__detail' }, dotted(res.detail)),
          ]),
        ]);
      }));
    },
  };
}
