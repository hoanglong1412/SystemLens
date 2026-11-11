/**
 * @file   performanceApi.js
 * @brief  Client for the "performance" endpoints (contract: api/openapi.yaml).
 */

/** Path of getPerformanceResources. */
export const PERFORMANCE_RESOURCES_PATH = '/api/v1/performance/resources';
/** Path of getCpuPerformance. */
export const PERFORMANCE_CPU_PATH = '/api/v1/performance/cpu';
/** Path of getCpuCores. */
export const PERFORMANCE_CPU_CORES_PATH = '/api/v1/performance/cpu/cores';
/** Path of getMemoryPerformance. */
export const PERFORMANCE_MEMORY_PATH = '/api/v1/performance/memory';
/** Path of getDiskPerformance. */
export const PERFORMANCE_DISKS_PATH = '/api/v1/performance/disks';
/** Path of getNetworkPerformance. */
export const PERFORMANCE_NETWORKS_PATH = '/api/v1/performance/networks';
/** Path of getGpuPerformance. */
export const PERFORMANCE_GPUS_PATH = '/api/v1/performance/gpus';

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

/**
 * Fetch the usage of every logical processor (operationId getCpuCores).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} CpuCores object of the contract.
 * @throws {import('../apiClient.js').ApiError}
 */
export function getCpuCores(client) {
  return client.get(PERFORMANCE_CPU_CORES_PATH);
}

/**
 * Fetch the data of the Memory screen (operationId getMemoryPerformance).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} MemoryPerformance object of the contract.
 * @throws {import('../apiClient.js').ApiError}
 */
export function getMemoryPerformance(client) {
  return client.get(PERFORMANCE_MEMORY_PATH);
}

/**
 * Fetch the detail of every disk (operationId getDiskPerformance).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object[]>} DiskDetail objects of the contract.
 * @throws {import('../apiClient.js').ApiError}
 */
export async function getDiskPerformance(client) {
  return (await client.get(PERFORMANCE_DISKS_PATH)).disks;
}

/**
 * Fetch the detail of every network adapter (operationId getNetworkPerformance).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object[]>} NetworkDetail objects of the contract.
 * @throws {import('../apiClient.js').ApiError}
 */
export async function getNetworkPerformance(client) {
  return (await client.get(PERFORMANCE_NETWORKS_PATH)).adapters;
}

/**
 * Fetch the detail of every graphics adapter (operationId getGpuPerformance).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object[]>} GpuDetail objects of the contract.
 * @throws {import('../apiClient.js').ApiError}
 */
export async function getGpuPerformance(client) {
  return (await client.get(PERFORMANCE_GPUS_PATH)).gpus;
}
