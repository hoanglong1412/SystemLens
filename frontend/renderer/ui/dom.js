/**
 * @file   dom.js
 * @brief  Tiny DOM builders used by every renderer module (shell, views, components).
 *
 * The page CSP (style-src 'self') blocks inline style="" attributes, so dynamic sizes and colours
 * are set through the CSSOM (element.style), which the CSP allows.
 */

const SVG_NS = 'http://www.w3.org/2000/svg';

/**
 * Apply props to an element.
 * @param {Element} el Target element.
 * @param {object} props
 *   class: string, text: string, style: object (CSSOM properties), on: {event: handler},
 *   anything else: set as attribute (skipped when null/undefined/false).
 */
function applyProps(el, props) {
  for (const [key, value] of Object.entries(props)) {
    if (value === null || value === undefined || value === false) continue;
    if (key === 'class') {
      el.setAttribute('class', value);
    } else if (key === 'text') {
      el.textContent = value;
    } else if (key === 'style') {
      Object.assign(el.style, value);
    } else if (key === 'on') {
      for (const [event, handler] of Object.entries(value)) el.addEventListener(event, handler);
    } else {
      el.setAttribute(key, value === true ? '' : String(value));
    }
  }
}

/**
 * Append children, skipping null/false so callers can write `cond && h(...)`.
 * @param {Element} el Parent.
 * @param {Array|Node|string} children Nodes or strings (strings become text nodes).
 */
function appendChildren(el, children) {
  for (const child of [children].flat(Infinity)) {
    if (child === null || child === undefined || child === false) continue;
    el.append(child instanceof Node ? child : document.createTextNode(String(child)));
  }
}

/**
 * Create an HTML element.
 * @param {string} tag Tag name.
 * @param {object} [props] See applyProps.
 * @param {Array|Node|string} [children]
 * @returns {HTMLElement}
 */
export function h(tag, props = {}, children = []) {
  const el = document.createElement(tag);
  applyProps(el, props);
  appendChildren(el, children);
  return el;
}

/**
 * Join items with a "·" separator, each separator in its own span so CSS can space it like the design
 * (e.g. "Version 10.0.19045 · User: ASUS").
 * @param {string[]} items Texts to join.
 * @returns {Array<Node|string>} Children for h().
 */
export function dotted(items) {
  return items.flatMap((item, i) => (i === 0 ? [item] : [h('span', { class: 'dot-sep', text: '·' }), item]));
}

/**
 * Create an SVG element.
 * @param {string} tag SVG tag name.
 * @param {object} [props] See applyProps.
 * @param {Array|Node} [children]
 * @returns {SVGElement}
 */
export function s(tag, props = {}, children = []) {
  const el = document.createElementNS(SVG_NS, tag);
  applyProps(el, props);
  appendChildren(el, children);
  return el;
}
