/**
 * @file   viewSwitcher.js
 * @brief  Shows one screen at a time in the shell content slot and provides the temporary toggle
 *         button that cycles between the registered screens.
 *
 * Each screen gets its own persistent host element: switching only hides/shows hosts, so a screen
 * keeps its state (e.g. the hello message filled in asynchronously) while hidden.
 */
import { h } from '../ui/dom.js';

export class ViewSwitcher {
  /**
   * @param {HTMLElement} content Shell content slot.
   * @param {Array<{id: string, label: string, render: (host: HTMLElement) => void}>} views
   *        Screens in toggle order; render is called once, the first time the screen is shown.
   */
  constructor(content, views) {
    this.content = content;
    this.views = views.map((view) => ({ ...view, host: null }));
    this.current = null;
    this.button = h('button', {
      class: 'btn btn--small view-toggle', type: 'button', on: { click: () => this.showNext() },
    });
  }

  /**
   * Show a screen by id, rendering it on first use.
   * @param {string} id View id.
   * @throws {Error} If no view has this id.
   */
  show(id) {
    const view = this.views.find((v) => v.id === id);
    if (!view) throw new Error(`Unknown view: ${id}`);
    if (!view.host) {
      view.host = h('div', { class: 'view-host' });
      this.content.append(view.host);
      view.render(view.host);
    }
    for (const v of this.views) v.host?.classList.toggle('view-host--hidden', v !== view);
    this.current = view;
    this.button.textContent = `⇄ ${this.#next().label}`;
  }

  /** Show the screen after the current one (wrapping around). */
  showNext() {
    this.show(this.#next().id);
  }

  /**
   * @returns {object} The view after the current one.
   */
  #next() {
    const index = this.views.indexOf(this.current);
    return this.views[(index + 1) % this.views.length];
  }
}
