/**
 * @file   liveDataLoop.js
 * @brief  Repeats an asynchronous load at a fixed pause and reports each result or failure.
 *         Used to refresh the UI from the backend once per second.
 */

export class LiveDataLoop {
  /**
   * @param {object} options
   * @param {() => Promise<any>} options.load Fetches one set of data.
   * @param {number} options.intervalMs Pause between the end of one load and the start of the next.
   * @param {(data: any) => void} options.onData Called with each successful result.
   * @param {(error: Error) => void} options.onError Called with each failure; the loop keeps going.
   */
  constructor({ load, intervalMs, onData, onError }) {
    this.load = load;
    this.intervalMs = intervalMs;
    this.onData = onData;
    this.onError = onError;
    this.timer = null;
    this.running = false;
  }

  /** Start loading immediately. Has no effect when already running. */
  start() {
    if (this.running) return;
    this.running = true;
    this.#cycle();
  }

  /** Stop after the load in flight (its result is dropped). */
  stop() {
    this.running = false;
    clearTimeout(this.timer);
    this.timer = null;
  }

  /**
   * One load followed by scheduling the next. The next load is scheduled only after this one
   * settled, so a slow backend never has overlapping requests queued up.
   */
  async #cycle() {
    try {
      const data = await this.load();
      if (this.running) this.onData(data);
    } catch (error) {
      if (this.running) this.onError(error);
    }
    if (this.running) {
      this.timer = setTimeout(() => this.#cycle(), this.intervalMs);
    }
  }
}
