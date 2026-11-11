/**
 * @file   pollingScreen.js
 * @brief  Wraps a screen so it refreshes from the backend only while it is visible: the ViewSwitcher
 *         calls onShow / onHide, which start and stop a LiveDataLoop.
 */
import { LiveDataLoop } from './liveDataLoop.js';

/**
 * Create a screen definition for the ViewSwitcher.
 * @param {object} options
 * @param {() => Promise<import('../api/apiClient.js').ApiClient>} options.getClient Resolves once the backend
 *        connection is known.
 * @param {number} options.intervalMs Pause between two refreshes.
 * @param {(client: import('../api/apiClient.js').ApiClient) => Promise<any>} options.load Fetch the data.
 * @param {(host: HTMLElement) => {update: (data: any) => void}} options.create Build the screen DOM in host.
 * @returns {{render: (host: HTMLElement) => void, onShow: () => void, onHide: () => void, refresh: () => void}}
 */
export function createPollingScreen({ getClient, intervalMs, load, create }) {
  let view = null;
  let loop = null;
  return {
    render(host) {
      view = create(host);
    },
    onShow() {
      if (loop) return;
      loop = new LiveDataLoop({
        intervalMs,
        load: async () => load(await getClient()),
        onData: (data) => view.update(data),
        // The app shell already reports a lost connection; here the last values simply stay.
        onError: (error) => console.error(error),
      });
      loop.start();
    },
    onHide() {
      loop?.stop();
      loop = null;
    },
    /** Reload now (e.g. after the user changed the selection). */
    refresh() {
      loop?.refresh();
    },
  };
}
