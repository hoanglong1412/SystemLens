/**
 * @file   searchBox.js
 * @brief  Text input used by the table screens to filter rows as the user types.
 */
import { h } from './dom.js';

/**
 * Create a filter input.
 * @param {object} options
 * @param {string} options.placeholder Hint shown while empty.
 * @param {(text: string) => void} options.onInput Called with the current text on every change.
 * @returns {HTMLInputElement}
 */
export function createSearchBox({ placeholder, onInput }) {
  const input = h('input', {
    class: 'search-box', type: 'search', placeholder, 'aria-label': placeholder, spellcheck: 'false',
  });
  input.addEventListener('input', () => onInput(input.value));
  return input;
}
