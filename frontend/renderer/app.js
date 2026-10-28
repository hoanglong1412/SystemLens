/**
 * @file   app.js
 * @brief  Renderer entry point: builds the app shell, registers the screens (sample greeting by
 *         default, Performance > CPU mock) and connects to the backend for the greeting.
 */
import { ApiClient } from './api/apiClient.js';
import { getSampleHello } from './api/sample/sampleHelloApi.js';
import { createAppShell } from './shell/appShell.js';
import { SHELL_MOCK } from './shell/shellMock.js';
import { ViewSwitcher } from './views/viewSwitcher.js';
import { renderHelloView } from './views/hello/helloView.js';
import { renderCpuPerformanceView } from './views/cpu/cpuPerformanceView.js';
import { CPU_PERFORMANCE_MOCK } from './views/cpu/cpuPerformanceMock.js';

/**
 * Show a message, or an error, in the hello screen.
 * @param {string} text Text to display.
 * @param {boolean} [isError] Style it as an error.
 */
function render(text, isError = false) {
  const element = document.getElementById('message');
  element.textContent = text;
  element.classList.toggle('error', isError);
}

/**
 * Build the shell and the screens, then show the hello screen.
 */
function mountUi() {
  const { content, actions } = createAppShell(document.getElementById('app'), SHELL_MOCK);
  const switcher = new ViewSwitcher(content, [
    { id: 'hello', label: 'Hello', render: (host) => renderHelloView(host, 'Connecting to backend...') },
    { id: 'cpu', label: 'CPU performance', render: (host) => renderCpuPerformanceView(host, CPU_PERFORMANCE_MOCK) },
  ]);
  actions.append(switcher.button);
  switcher.show('hello');
}

/**
 * Get the backend connection info from the main process, call the sample API and render the result.
 */
async function bootstrap() {
  mountUi();
  try {
    const config = await window.systemlens.getBackendConfig();
    const client = new ApiClient(config);
    render(await getSampleHello(client));
  } catch (err) {
    console.error(err);
    render(`Cannot reach backend: ${err.message}`, true);
  }
}

bootstrap();
