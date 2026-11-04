/**
 * @file   appShell.js
 * @brief  Fixed frame shared by every screen: title bar, header (machine + quick stats), tab bar
 *         and status bar. Screens render only into the content slot returned by createAppShell.
 *
 * The frame (with its info icons) is built once; update() only refills the machine text, the quick
 * stats and the status texts, so a tooltip never loses its icon while open.
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
  return h('div', { class: `chip chip--${id}`, 'data-testid': `chip-${id}` }, [
    fill,
    h('span', { class: 'chip__label', text: label }),
    h('span', { class: 'chip__value', text: value }),
  ]);
}

/**
 * Quick stats chips.
 * @param {object} quickStats SHELL_MOCK.quickStats-shaped data.
 * @returns {HTMLElement[]}
 */
function quickStatChips(quickStats) {
  return [
    h('div', { class: 'chip chip--uptime', text: quickStats.uptime, 'data-testid': 'chip-uptime' }),
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
 * Build the app shell inside a container.
 * @param {HTMLElement} container Element that receives the shell (emptied first).
 * @param {object} data SHELL_MOCK-shaped data (see shellModel.js).
 * @returns {{content: HTMLElement, actions: HTMLElement, update: (data: object) => void}}
 *          content: slot for the current screen; actions: slot in the tab bar for extra buttons;
 *          update: refill the live parts with new data.
 */
export function createAppShell(container, data) {
  const { tooltips } = data;
  const content = h('main', { class: 'shell__content' });
  const actions = h('div', { class: 'tabbar__actions' });

  const machineName = h('span', { class: 'header__machine-text', 'data-testid': 'machine-name' });
  const machineDetails = h('div', { class: 'header__machine-details', 'data-testid': 'machine-details' });
  const quickStats = h('div', { class: 'quick-stats', 'data-testid': 'quick-stats' });
  const statusDot = h('span', { class: 'statusbar__dot' });
  const statusText = h('span', { class: 'statusbar__text', 'data-testid': 'status-connection' });
  const statusBoot = h('span', { class: 'statusbar__boot', 'data-testid': 'status-boot' });

  container.replaceChildren(
    titleBar(data.windowTitle),
    h('div', { class: 'header' }, [
      h('div', { class: 'header__machine-icon' }, [monitorIcon()]),
      h('div', { class: 'header__machine' }, [
        h('div', { class: 'header__machine-name' }, [machineName, infoIcon(tooltips.machine, 'header__info')]),
        machineDetails,
      ]),
      infoIcon(tooltips.quickStats, 'quick-stats__info'),
      quickStats,
    ]),
    tabBar(data, actions),
    content,
    h('div', { class: 'statusbar' }, [
      statusDot,
      statusText,
      infoIcon(tooltips.status, 'statusbar__info'),
      statusBoot,
      h('button', { class: 'btn btn--link', type: 'button', text: data.status.action }),
    ]),
  );
  container.classList.add('shell');

  /**
   * Refill machine identity, quick stats and status bar.
   * @param {object} next SHELL_MOCK-shaped data; status.connected false shows the offline dot.
   */
  function update(next) {
    machineName.replaceChildren(...dotted([next.machine.name, next.machine.os]));
    machineDetails.replaceChildren(...dotted(next.machine.details));
    quickStats.replaceChildren(...quickStatChips(next.quickStats));
    statusText.replaceChildren(...dotted(next.status.connection));
    statusDot.classList.toggle('statusbar__dot--offline', next.status.connected === false);
    statusBoot.textContent = next.status.bootTime;
  }

  update(data);
  return { content, actions, update };
}
