/**
 * @file   cpuApi.js
 * @brief  Client for GET /api/v1/cpu/info and GET /api/v1/cpu/metrics (contract: api/openapi.yaml,
 *         schemas CpuInfo and CpuMetrics).
 */

/** Path of the static CPU facts. */
export const CPU_INFO_PATH = '/api/v1/cpu/info';
/** Path of the live CPU metrics. */
export const CPU_METRICS_PATH = '/api/v1/cpu/metrics';

/**
 * Fetch the static processor facts.
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} CpuInfo.
 * @throws {import('../apiClient.js').ApiError} 501 when the platform has no CPU provider.
 */
export function getCpuInfo(client) {
  return client.get(CPU_INFO_PATH);
}

/**
 * Fetch the metrics of the last interval and the 60-sample history.
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} CpuMetrics.
 * @throws {import('../apiClient.js').ApiError} 503 before the first interval, 501 without CPU provider.
 */
export function getCpuMetrics(client) {
  return client.get(CPU_METRICS_PATH);
}
