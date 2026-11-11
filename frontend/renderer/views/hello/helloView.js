/**
 * @file   helloView.js
 * @brief  Sample screen: a single centred message (the greeting from the sample API, or an error).
 *         The message can be set before the screen is first shown; it is displayed when it is rendered.
 */
import { h } from '../../ui/dom.js';

/**
 * Create the hello screen.
 * @param {string} initialText Message shown until setMessage() is called.
 * @returns {{render: (host: HTMLElement) => void, setMessage: (text: string, isError?: boolean) => void}}
 */
export function createHelloView(initialText) {
  let text = initialText;
  let isError = false;
  let element = null;

  /** Show the current message in the DOM, if the screen has been rendered. */
  function apply() {
    if (!element) return;
    element.textContent = text;
    element.classList.toggle('error', isError);
  }

  return {
    render(host) {
      element = h('h1', { id: 'message', class: 'hello__message' });
      host.replaceChildren(h('div', { class: 'hello' }, [element]));
      apply();
    },
    setMessage(nextText, nextIsError = false) {
      text = nextText;
      isError = nextIsError;
      apply();
    },
  };
}
