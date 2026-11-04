/**
 * @file   perCoreHistory.js
 * @brief  Usage history per logical processor, accumulated on the client: CpuMetrics only carries the
 *         current per-core values, and the Per-core grid needs a short chart for each core.
 */

export class PerCoreHistory {
  /**
   * @param {number} capacity Samples kept per core.
   */
  constructor(capacity) {
    this.capacity = capacity;
    this.series = [];
    this.lastKey = null;
  }

  /**
   * Append one sample per core. The same metrics response (same uptime) is only counted once, so
   * re-rendering without a new sample does not duplicate points.
   * @param {object} current CpuMetrics.current.
   */
  push(current) {
    const key = current.uptimeSeconds;
    if (key === this.lastKey) return;
    this.lastKey = key;
    current.perCoreUsagePercent.forEach((value, i) => {
      const values = this.series[i] ?? (this.series[i] = []);
      values.push(value);
      if (values.length > this.capacity) values.shift();
    });
    // Cores that went offline are dropped.
    this.series.length = current.perCoreUsagePercent.length;
  }

  /**
   * @returns {number[][]} One array per core, oldest first.
   */
  values() {
    return this.series;
  }
}
