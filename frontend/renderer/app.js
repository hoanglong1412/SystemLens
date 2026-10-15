/**
 * @file   app.js
 * @brief  Renderer entry point: connects to the backend and shows the sample greeting.
 */
import { ApiClient } from './api/apiClient.js';
import { getSampleHello } from './api/sample/sampleHelloApi.js';

/**
 * Show a message, or an error, in the page.
 * @param {string} text Text to display.
 * @param {boolean} [isError] Style it as an error.
 */
function render(text, isError = false) {
  const element = document.getElementById('message');
  element.textContent = text;
  element.classList.toggle('error', isError);
}

/**
 * Get the backend connection info from the main process, call the sample API and render the result.
 */
async function bootstrap() {
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
