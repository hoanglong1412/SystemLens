/**
 * @file   performanceApi.js
 * @brief  Client for the "performance" endpoints (contract: api/openapi.yaml).
 */

/** Path of getPerformanceResources. */
export const PERFORMANCE_RESOURCES_PATH = '/api/v1/performance/resources';
/** Path of getCpuPerformance. */
export const PERFORMANCE_CPU_PATH = '/api/v1/performance/cpu';

/**
 * Fetch every monitored resource with its sparkline (operationId getPerformanceResources).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object[]>} PerformanceResource objects of the contract, in display order.
 * @throws {import('../apiClient.js').ApiError}
 */
export async function getPerformanceResources(client) {
  const body = await client.get(PERFORMANCE_RESOURCES_PATH);
  return body.resources;
}

/**
 * Fetch the data of the Performance > CPU screen (operationId getCpuPerformance).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} CpuPerformance object of the contract.
 * @throws {import('../apiClient.js').ApiError}
 */
export function getCpuPerformance(client) {
  return client.get(PERFORMANCE_CPU_PATH);
}
