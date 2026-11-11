/**
 * @file   app.js
 * @brief  Renderer entry point: builds the app shell, registers one screen per tab (plus the sample hello
 *         screen behind a toggle button), connects to the backend and keeps the shell refreshed.
 *         Each screen refreshes itself while it is visible (see live/pollingScreen.js).
 */
import { ApiClient } from './api/apiClient.js';
import { getSampleHello } from './api/sample/sampleHelloApi.js';
import { getSystemOverview } from './api/system/systemApi.js';
import { LiveDataLoop } from './live/liveDataLoop.js';
import { createAppShell } from './shell/appShell.js';
import { toShellData } from './shell/shellModel.js';
import { h } from './ui/dom.js';
import { ViewSwitcher } from './views/viewSwitcher.js';
import { createHelloView } from './views/hello/helloView.js';
import { createPerformanceScreen } from './views/performance/performanceScreen.js';
import { createProcessesScreen } from './views/processes/processesScreen.js';
import { createDetailsScreen } from './views/processes/detailsScreen.js';
import { createAppHistoryScreen } from './views/history/appHistoryScreen.js';
import { createServicesScreen } from './views/inventory/servicesScreen.js';
import { createStartupScreen } from './views/inventory/startupScreen.js';
import { createUsersScreen } from './views/inventory/usersScreen.js';

/** Pause between two refreshes of the shell; matches the backend sample interval ("Normal (1 s)"). */
const SHELL_REFRESH_INTERVAL_MS = 1000;
/** Tab shown when the application starts. */
const DEFAULT_TAB_ID = 'performance';
const HELLO_VIEW_ID = 'hello';
const HELLO_TOGGLE_TEXT = '⇄ Hello';
const HELLO_BACK_TEXT = '⇄ Back';

/**
 * Build the shell and register the screens. Everything shows "no value" until data arrives.
 * @param {() => Promise<ApiClient>} getClient Resolves once the backend connection is known.
 * @param {ReturnType<typeof createHelloView>} hello The sample greeting screen.
 * @returns {{showOverview: (overview: object) => void, showDisconnected: () => void}}
 *          Functions that push the result of a shell refresh into the UI.
 */
function mountUi(getClient, hello) {
  let activeTabId = DEFAULT_TAB_ID;
  let overview = null;
  let connected = false;

  /** Redraw the shell with the current overview, connection state and active tab. */
  const refreshShell = () => shell.update(toShellData(overview, connected, activeTabId));

  const shell = createAppShell(document.getElementById('app'), toShellData(null, false, activeTabId), {
    onTabSelect: (tabId) => {
      activeTabId = tabId;
      switcher.show(tabId);
      refreshShell();
      toggle.textContent = HELLO_TOGGLE_TEXT;
    },
  });

  const switcher = new ViewSwitcher(shell.content, [
    { id: HELLO_VIEW_ID, render: hello.render },
    { id: 'processes', ...createProcessesScreen({ getClient }) },
    { id: 'performance', ...createPerformanceScreen({ getClient }) },
    { id: 'history', ...createAppHistoryScreen({ getClient }) },
    { id: 'startup', ...createStartupScreen({ getClient }) },
    { id: 'users', ...createUsersScreen({ getClient }) },
    { id: 'details', ...createDetailsScreen({ getClient }) },
    { id: 'services', ...createServicesScreen({ getClient }) },
  ]);

  // Temporary button (not in the design): switches between the active tab and the sample greeting.
  const toggle = h('button', {
    class: 'btn btn--small view-toggle',
    type: 'button',
    text: HELLO_TOGGLE_TEXT,
    on: {
      click: () => {
        const onHello = switcher.currentId === HELLO_VIEW_ID;
        switcher.show(onHello ? activeTabId : HELLO_VIEW_ID);
        toggle.textContent = onHello ? HELLO_TOGGLE_TEXT : HELLO_BACK_TEXT;
      },
    },
  });
  shell.actions.append(toggle);
  switcher.show(activeTabId);

  return {
    showOverview: (next) => {
      overview = next;
      connected = true;
      refreshShell();
    },
    showDisconnected: () => {
      // The last values stay on screen; only the connection state changes.
      connected = false;
      refreshShell();
    },
  };
}

/**
 * Start refreshing the shell (header, quick stats, tabs, status bar) from the backend.
 * @param {ApiClient} client Configured API client.
 * @param {ReturnType<typeof mountUi>} ui Functions returned by mountUi.
 */
function startShellRefresh(client, ui) {
  new LiveDataLoop({
    intervalMs: SHELL_REFRESH_INTERVAL_MS,
    load: () => getSystemOverview(client),
    onData: ui.showOverview,
    onError: (error) => {
      console.error(error);
      ui.showDisconnected();
    },
  }).start();
}

/** Mount the UI, load the backend config, show the greeting and start the shell refresh. */
async function bootstrap() {
  const hello = createHelloView('Connecting to backend...');
  let resolveClient;
  let rejectClient;
  const clientReady = new Promise((resolve, reject) => {
    resolveClient = resolve;
    rejectClient = reject;
  });
  // The screens await this promise; a failure is reported in the hello screen, not as an unhandled rejection.
  clientReady.catch(() => {});

  const ui = mountUi(() => clientReady, hello);
  let client;
  try {
    client = new ApiClient(await window.systemlens.getBackendConfig());
    resolveClient(client);
  } catch (err) {
    console.error(err);
    rejectClient(err);
    hello.setMessage(`Cannot reach backend: ${err.message}`, true);
    return;
  }

  startShellRefresh(client, ui);
  try {
    hello.setMessage(await getSampleHello(client));
  } catch (err) {
    console.error(err);
    hello.setMessage(`Cannot reach backend: ${err.message}`, true);
  }
}

bootstrap();
