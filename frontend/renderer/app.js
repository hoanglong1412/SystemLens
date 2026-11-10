/**
 * @file   app.js
 * @brief  Renderer entry point: builds the app shell, registers the screens (sample greeting by
 *         default, Performance > CPU), connects to the backend and keeps the shell and the CPU
 *         screen refreshed with live data.
 */
import { ApiClient } from './api/apiClient.js';
import { getCpuPerformance, getPerformanceResources } from './api/performance/performanceApi.js';
import { getSampleHello } from './api/sample/sampleHelloApi.js';
import { getSystemOverview } from './api/system/systemApi.js';
import { LiveDataLoop } from './live/liveDataLoop.js';
import { createAppShell } from './shell/appShell.js';
import { toShellData } from './shell/shellModel.js';
import { ViewSwitcher } from './views/viewSwitcher.js';
import { renderHelloView } from './views/hello/helloView.js';
import { toCpuViewData } from './views/cpu/cpuPerformanceModel.js';
import { createCpuPerformanceView } from './views/cpu/cpuPerformanceView.js';

/** Pause between two refreshes; matches the backend sample interval ("Normal (1 s)"). */
const REFRESH_INTERVAL_MS = 1000;

/**
 * Show a text in the hello screen.
 * @param {string} text Text to display.
 * @param {boolean} [isError=false] Style the text as an error.
 */
function render(text, isError = false) {
  const element = document.getElementById('message');
  element.textContent = text;
  element.classList.toggle('error', isError);
}

/**
 * Build the shell and register the screens. Everything shows "no value" until data arrives.
 * @returns {{showOverview: (overview: object|null, connected: boolean) => void,
 *            showCpu: (resources: object[]|null, cpu: object|null) => void}}
 *          Functions that push new API data into the shell and the CPU screen.
 */
function mountUi() {
  const shell = createAppShell(document.getElementById('app'), toShellData(null, false));

  // The CPU screen is created lazily (first time it is shown), so the latest data is kept
  // here and handed over when the screen appears.
  let cpuView = null;
  let cpuData = toCpuViewData(null, null);

  const switcher = new ViewSwitcher(shell.content, [
    { id: 'hello', label: 'Hello', render: (host) => renderHelloView(host, 'Connecting to backend...') },
    { id: 'cpu', label: 'CPU performance', render: (host) => { cpuView = createCpuPerformanceView(host, cpuData); } },
  ]);
  shell.actions.append(switcher.button);
  switcher.show('hello');

  return {
    showOverview: (overview, connected) => shell.update(toShellData(overview, connected)),
    showCpu: (resources, cpu) => {
      cpuData = toCpuViewData(resources, cpu);
      cpuView?.update(cpuData);
    },
  };
}

/**
 * Start refreshing the shell and the CPU screen from the backend.
 * On a failed refresh the last values stay on screen and the status bar reports the disconnection;
 * the loop keeps retrying.
 * @param {ApiClient} client Configured API client.
 * @param {ReturnType<typeof mountUi>} ui Functions returned by mountUi.
 */
function startLiveData(client, ui) {
  let lastOverview = null;
  const loop = new LiveDataLoop({
    intervalMs: REFRESH_INTERVAL_MS,
    load: async () => {
      const [overview, resources, cpu] = await Promise.all([
        getSystemOverview(client),
        getPerformanceResources(client),
        getCpuPerformance(client),
      ]);
      return { overview, resources, cpu };
    },
    onData: ({ overview, resources, cpu }) => {
      lastOverview = overview;
      ui.showOverview(overview, true);
      ui.showCpu(resources, cpu);
    },
    onError: (error) => {
      console.error(error);
      ui.showOverview(lastOverview, false);
    },
  });
  loop.start();
}

/** Mount the UI, load the backend config, show the greeting and start the live data refresh. */
async function bootstrap() {
  const ui = mountUi();
  let client;
  try {
    const config = await window.systemlens.getBackendConfig();
    client = new ApiClient(config);
  } catch (err) {
    console.error(err);
    render(`Cannot reach backend: ${err.message}`, true);
    return;
  }

  startLiveData(client, ui);
  try {
    render(await getSampleHello(client));
  } catch (err) {
    console.error(err);
    render(`Cannot reach backend: ${err.message}`, true);
  }
}

bootstrap();
