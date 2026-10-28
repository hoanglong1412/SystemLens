/**
 * @file   appShell.js
 * @brief  Fixed frame shared by every screen: title bar, header (machine + quick stats), tab bar
 *         and status bar. Screens render only into the content slot returned by createAppShell.
 */
import { dotted, h } from '../ui/dom.js';
import { infoIcon } from '../ui/tooltip.js';
import { appLogoIcon, monitorIcon, sunIcon, windowControlIcon } from '../ui/icons.js';

/**
 * Title bar: app logo, window title and (decorative) window controls.
 * The real controls come from the native title bar overlay (see main.js); these glyphs sit under it.
 * @param {string} title Window title.
 * @returns {HTMLElement}
 */
function titleBar(title) {
  return h('div', { class: 'titlebar' }, [
    appLogoIcon(),
    h('span', { class: 'titlebar__title', text: title }),
    h('div', { class: 'titlebar__controls' }, ['minimize', 'maximize', 'close'].map(windowControlIcon)),
  ]);
}

/**
 * One quick-stat chip with a coloured fill proportional to its percent.
 * @param {{id: string, label: string, value: string, percent: number}} chip
 * @returns {HTMLElement}
 */
function percentChip({ id, label, value, percent }) {
  const fill = h('span', { class: 'chip__fill' });
  fill.style.width = `${percent}%`;
  return h('div', { class: `chip chip--${id}` }, [
    fill,
    h('span', { class: 'chip__label', text: label }),
    h('span', { class: 'chip__value', text: value }),
  ]);
}

/**
 * Header: machine identity on the left, quick stats on the right.
 * @param {object} data SHELL_MOCK-shaped data.
 * @returns {HTMLElement}
 */
function header(data) {
  const { machine, quickStats, tooltips } = data;
  return h('div', { class: 'header' }, [
    h('div', { class: 'header__machine-icon' }, [monitorIcon()]),
    h('div', { class: 'header__machine' }, [
      h('div', { class: 'header__machine-name' }, [
        ...dotted([machine.name, machine.os]),
        infoIcon(tooltips.machine, 'header__info'),
      ]),
      h('div', { class: 'header__machine-details' }, dotted(machine.details)),
    ]),
    infoIcon(tooltips.quickStats, 'quick-stats__info'),
    h('div', { class: 'quick-stats' }, [
      h('div', { class: 'chip chip--uptime', text: quickStats.uptime }),
      ...quickStats.chips.map(percentChip),
      h('div', { class: 'chip chip--network' }, [
        h('span', { class: 'chip__label', text: `↑ ${quickStats.network.up}` }),
        h('span', { class: 'chip__label chip__label--down', text: `↓ ${quickStats.network.down}` }),
      ]),
      h('div', { class: 'chip chip--top' }, [
        h('span', { class: 'chip__label', text: quickStats.topProcess.label }),
        h('span', { class: 'chip__value', text: quickStats.topProcess.value }),
      ]),
    ]),
  ]);
}

/**
 * Tab bar with the screen tabs and the right-side actions.
 * @param {object} data SHELL_MOCK-shaped data.
 * @param {HTMLElement} actions Container for extra action buttons (filled by the caller).
 * @returns {HTMLElement}
 */
function tabBar(data, actions) {
  return h('div', { class: 'tabbar' }, [
    h('div', { class: 'tabbar__tabs' }, data.tabs.map((name) =>
      h('div', { class: name === data.activeTab ? 'tab tab--active' : 'tab', text: name }))),
    actions,
    h('button', { class: 'btn btn--small tabbar__export', type: 'button', text: 'Export snapshot' }),
    h('button', { class: 'btn btn--icon', type: 'button', 'aria-label': 'Toggle theme' }, [sunIcon()]),
  ]);
}

/**
 * Status bar: backend connection, boot time and the Desktop Monitor action.
 * @param {object} data SHELL_MOCK-shaped data.
 * @returns {HTMLElement}
 */
function statusBar(data) {
  const { status, tooltips } = data;
  return h('div', { class: 'statusbar' }, [
    h('span', { class: 'statusbar__dot' }),
    h('span', { class: 'statusbar__text' }, dotted(status.connection)),
    infoIcon(tooltips.status, 'statusbar__info'),
    h('span', { class: 'statusbar__boot', text: status.bootTime }),
    h('button', { class: 'btn btn--link', type: 'button', text: status.action }),
  ]);
}

/**
 * Build the app shell inside a container.
 * @param {HTMLElement} container Element that receives the shell (emptied first).
 * @param {object} data SHELL_MOCK-shaped data.
 * @returns {{content: HTMLElement, actions: HTMLElement}} content: slot for the current screen;
 *          actions: slot in the tab bar for extra buttons.
 */
export function createAppShell(container, data) {
  const content = h('main', { class: 'shell__content' });
  const actions = h('div', { class: 'tabbar__actions' });
  container.replaceChildren(
    titleBar(data.windowTitle),
    header(data),
    tabBar(data, actions),
    content,
    statusBar(data),
  );
  container.classList.add('shell');
  return { content, actions };
}
