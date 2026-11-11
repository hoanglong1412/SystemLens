/**
 * @file   viewSwitcher.js
 * @brief  Shows one screen at a time in the shell content slot.
 *
 * Each screen gets its own persistent host element: switching only hides/shows hosts, so a screen
 * keeps its state (sort order, filter text, scroll position) while hidden. Screens that refresh from
 * the backend are told when they appear and disappear (onShow / onHide) so they poll only while visible.
 */
import { h } from '../ui/dom.js';

export class ViewSwitcher {
  /**
   * @param {HTMLElement} content Shell content slot.
   * @param {Array<{id: string, render: (host: HTMLElement) => void, onShow?: () => void, onHide?: () => void}>} views
   *        Screens; render is called once, the first time the screen is shown.
   */
  constructor(content, views) {
    this.content = content;
    this.views = views.map((view) => ({ ...view, host: null }));
    this.current = null;
  }

  /**
   * Id of the screen on display.
   * @returns {string|null}
   */
  get currentId() {
    return this.current?.id ?? null;
  }

  /**
   * Show a screen by id, rendering it on first use.
   * @param {string} id View id.
   * @throws {Error} If no view has this id.
   */
  show(id) {
    const view = this.views.find((v) => v.id === id);
    if (!view) throw new Error(`Unknown view: ${id}`);
    if (view === this.current) return;
    this.current?.onHide?.();
    if (!view.host) {
      view.host = h('div', { class: 'view-host' });
      this.content.append(view.host);
      view.render(view.host);
    }
    for (const v of this.views) v.host?.classList.toggle('view-host--hidden', v !== view);
    this.current = view;
    view.onShow?.();
  }
}
