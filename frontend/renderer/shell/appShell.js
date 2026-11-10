/**
 * @file   appShell.js
 * @brief  Fixed frame shared by every screen: title bar, header (machine + quick stats), tab bar
 *         and status bar. Screens render only into the content slot returned by createAppShell.
 *
 * The frame is built once; update() only replaces the texts and chips that change, so hover
 * state (tooltips) and focus survive the once-per-second refresh.
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
  fill.style.width = `${Math.min(Math.max(percent, 0), 100)}%`;
  return h('div', { class: `chip chip--${id}` }, [
    fill,
    h('span', { class: 'chip__label', text: label }),
    h('span', { class: 'chip__value', text: value }),
  ]);
}

/**
 * The quick-stat chips: uptime, CPU / memory / disk, network and top process.
 * @param {object} quickStats
 * @returns {HTMLElement[]}
 */
function quickStatChips(quickStats) {
  return [
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
  ];
}

/**
 * Header: machine identity on the left, quick stats on the right.
 * @param {object} tooltips Static tooltip texts.
 * @returns {{element: HTMLElement, update: (data: object) => void}}
 */
function header(tooltips) {
  const machineText = h('span', { class: 'slot' });
  const machineDetails = h('div', { class: 'header__machine-details' });
  const quickStats = h('div', { class: 'quick-stats' });
  const element = h('div', { class: 'header' }, [
    h('div', { class: 'header__machine-icon' }, [monitorIcon()]),
    h('div', { class: 'header__machine' }, [
      h('div', { class: 'header__machine-name' }, [machineText, infoIcon(tooltips.machine, 'header__info')]),
      machineDetails,
    ]),
    infoIcon(tooltips.quickStats, 'quick-stats__info'),
    quickStats,
  ]);
  return {
    element,
    update({ machine, quickStats: stats }) {
      machineText.replaceChildren(...dotted([machine.name, machine.os]));
      machineDetails.replaceChildren(...dotted(machine.details));
      quickStats.replaceChildren(...quickStatChips(stats));
    },
  };
}

/**
 * Tab bar with the screen tabs and the right-side actions.
 * @param {HTMLElement} actions Container for extra action buttons (filled by the caller).
 * @returns {{element: HTMLElement, update: (data: object) => void}}
 */
function tabBar(actions) {
  const tabs = h('div', { class: 'tabbar__tabs' });
  const element = h('div', { class: 'tabbar' }, [
    tabs,
    actions,
    h('button', { class: 'btn btn--small tabbar__export', type: 'button', text: 'Export snapshot' }),
    h('button', { class: 'btn btn--icon', type: 'button', 'aria-label': 'Toggle theme' }, [sunIcon()]),
  ]);
  return {
    element,
    update(data) {
      tabs.replaceChildren(...data.tabs.map((name) =>
        h('div', { class: name.startsWith(data.activeTab) ? 'tab tab--active' : 'tab', text: name })));
    },
  };
}

/**
 * Status bar: backend connection, boot time and the Desktop Monitor action.
 * @param {object} data Initial shell data (static action text and tooltips).
 * @returns {{element: HTMLElement, update: (data: object) => void}}
 */
function statusBar(data) {
  const dot = h('span', { class: 'statusbar__dot' });
  const connection = h('span', { class: 'statusbar__text' });
  const bootTime = h('span', { class: 'statusbar__boot' });
  const element = h('div', { class: 'statusbar' }, [
    dot,
    connection,
    infoIcon(data.tooltips.status, 'statusbar__info'),
    bootTime,
    h('button', { class: 'btn btn--link', type: 'button', text: data.status.action }),
  ]);
  return {
    element,
    update({ status }) {
      dot.classList.toggle('statusbar__dot--offline', !status.connected);
      connection.replaceChildren(...dotted(status.connection));
      bootTime.textContent = status.bootTime;
    },
  };
}

/**
 * Build the app shell inside a container.
 * @param {HTMLElement} container Element that receives the shell (emptied first).
 * @param {object} data Shell display data (see shellModel.js, toShellData).
 * @returns {{content: HTMLElement, actions: HTMLElement, update: (data: object) => void}}
 *          content: slot for the current screen; actions: slot in the tab bar for extra buttons;
 *          update: show new shell data.
 */
export function createAppShell(container, data) {
  const content = h('main', { class: 'shell__content' });
  const actions = h('div', { class: 'tabbar__actions' });
  const parts = [header(data.tooltips), tabBar(actions), statusBar(data)];
  const [headerPart, tabBarPart, statusBarPart] = parts;
  container.replaceChildren(
    titleBar(data.windowTitle),
    headerPart.element,
    tabBarPart.element,
    content,
    statusBarPart.element,
  );
  container.classList.add('shell');

  const update = (next) => parts.forEach((part) => part.update(next));
  update(data);
  return { content, actions, update };
}
