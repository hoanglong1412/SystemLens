/**
 * @file   app.js
 * @brief  Renderer entry point: builds the app shell and the screens (Performance > CPU by default,
 *         sample greeting), connects to the backend and refreshes the CPU data every second.
 */
import { ApiClient } from './api/apiClient.js';
import { getSampleHello } from './api/sample/sampleHelloApi.js';
import { getCpuInfo, getCpuMetrics } from './api/cpu/cpuApi.js';
import { getSystemInfo } from './api/system/systemApi.js';
import { createAppShell } from './shell/appShell.js';
import { SHELL_MOCK } from './shell/shellMock.js';
import { buildShellModel } from './shell/shellModel.js';
import { ViewSwitcher } from './views/viewSwitcher.js';
import { renderHelloView } from './views/hello/helloView.js';
import { createCpuPerformanceView } from './views/cpu/cpuPerformanceView.js';
import { CPU_PERFORMANCE_MOCK } from './views/cpu/cpuPerformanceMock.js';
import { HISTORY_CAPACITY, buildCpuViewModel } from './views/cpu/cpuPerformanceModel.js';
import { PerCoreHistory } from './views/cpu/perCoreHistory.js';

/** Time between two metrics requests (the backend samples every 1000 ms). */
const POLL_INTERVAL_MS = 1000;
/** HTTP statuses that mean "this platform has no such data": stop asking for it. */
const STATUS_NOT_IMPLEMENTED = 501;
/** "Measured soon": not a connection problem. */
const STATUS_SERVICE_UNAVAILABLE = 503;

/** Everything the screens show, refreshed by the polling loop. */
const state = {
  system: null,
  cpuInfo: null,
  cpuInfoUnavailable: false,
  systemUnavailable: false,
  metrics: null,
  connected: true,
  perCore: new PerCoreHistory(HISTORY_CAPACITY),
};

let shell = null;
let cpuView = null;

/**
 * Show a message, or an error, in the hello screen.
 * @param {string} text Text to display.
 * @param {boolean} [isError] Style it as an error.
 */
function renderHello(text, isError = false) {
  const element = document.getElementById('message');
  element.textContent = text;
  element.classList.toggle('error', isError);
}

/** Re-render the shell and the CPU screen from state. */
function render() {
  shell.update(buildShellModel({
    system: state.system, metrics: state.metrics, connected: state.connected, mock: SHELL_MOCK,
  }));
  cpuView?.update(buildCpuViewModel({
    info: state.cpuInfo, metrics: state.metrics, perCoreHistory: state.perCore.values(), mock: CPU_PERFORMANCE_MOCK,
  }));
}

/**
 * Build the shell and the screens, then show the CPU screen.
 */
function mountUi() {
  shell = createAppShell(document.getElementById('app'), buildShellModel({
    system: null, metrics: null, connected: true, mock: SHELL_MOCK,
  }));
  const switcher = new ViewSwitcher(shell.content, [
    {
      id: 'cpu', label: 'CPU performance',
      render: (host) => {
        cpuView = createCpuPerformanceView(host, buildCpuViewModel({
          info: state.cpuInfo, metrics: state.metrics, mock: CPU_PERFORMANCE_MOCK,
        }));
      },
    },
    { id: 'hello', label: 'Hello', render: (host) => renderHelloView(host, 'Connecting to backend...') },
  ]);
  shell.actions.append(switcher.button);
  switcher.show('cpu');
  // Rendered hidden up front so the greeting can be written as soon as it arrives.
  switcher.prerender('hello');
}

/**
 * Load a resource that only has to be read once; 501 marks it unavailable for good.
 * @param {() => Promise<object>} load API call.
 * @param {string} key state key receiving the value.
 * @param {string} unavailableKey state key set when the platform cannot provide it.
 */
async function loadOnce(load, key, unavailableKey) {
  if (state[key] || state[unavailableKey]) return;
  try {
    state[key] = await load();
  } catch (err) {
    if (err.status === STATUS_NOT_IMPLEMENTED) state[unavailableKey] = true;
    else throw err;
  }
}

/**
 * One polling step: static data if still missing, then the metrics. Never throws.
 * @param {ApiClient} client
 */
async function poll(client) {
  try {
    await loadOnce(() => getSystemInfo(client), 'system', 'systemUnavailable');
    await loadOnce(() => getCpuInfo(client), 'cpuInfo', 'cpuInfoUnavailable');
    if (!state.cpuInfoUnavailable) {
      state.metrics = await getCpuMetrics(client);
      state.perCore.push(state.metrics.current);
    }
    state.connected = true;
  } catch (err) {
    // 503 only means the first interval is not measured yet; anything else is a lost backend.
    state.connected = err.status === STATUS_SERVICE_UNAVAILABLE;
    if (!state.connected) console.error(err);
  }
  render();
}

/**
 * Poll forever; the next request is scheduled after the previous one finished, so slow responses
 * never pile up.
 * @param {ApiClient} client
 */
async function pollLoop(client) {
  await poll(client);
  setTimeout(() => pollLoop(client), POLL_INTERVAL_MS);
}

/**
 * Get the backend connection info from the main process, show the greeting and start polling.
 */
async function bootstrap() {
  mountUi();
  let client;
  try {
    client = new ApiClient(await window.systemlens.getBackendConfig());
  } catch (err) {
    console.error(err);
    state.connected = false;
    render();
    renderHello(`Cannot reach backend: ${err.message}`, true);
    return;
  }

  pollLoop(client);
  try {
    renderHello(await getSampleHello(client));
  } catch (err) {
    console.error(err);
    renderHello(`Cannot reach backend: ${err.message}`, true);
  }
}

bootstrap();
