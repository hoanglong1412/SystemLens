/**
 * @file   helloView.js
 * @brief  Sample screen: a single centred message (the greeting from the sample API, or an error).
 */
import { h } from '../../ui/dom.js';

/**
 * Render the hello screen with an initial text.
 * @param {HTMLElement} container Shell content slot (emptied first).
 * @param {string} text Initial message.
 * @returns {HTMLElement} The message element, so the caller can update it later.
 */
export function renderHelloView(container, text) {
  const message = h('h1', { id: 'message', class: 'hello__message', text });
  container.replaceChildren(h('div', { class: 'hello' }, [message]));
  return message;
}
