/**
 * @file   processApi.js
 * @brief  Client for the "processes" endpoints (contract: api/openapi.yaml).
 */

/** Path of getProcesses. */
export const PROCESSES_PATH = '/api/v1/processes';
/** Path of getAppHistory. */
export const APP_HISTORY_PATH = '/api/v1/apps/history';

/**
 * Fetch every process (operationId getProcesses).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object[]>} ProcessEntry objects of the contract, highest CPU first.
 * @throws {import('../apiClient.js').ApiError}
 */
export async function getProcesses(client) {
  const body = await client.get(PROCESSES_PATH);
  return body.processes;
}

/**
 * Fetch the usage per application since the backend started (operationId getAppHistory).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} AppHistory object of the contract.
 * @throws {import('../apiClient.js').ApiError}
 */
export function getAppHistory(client) {
  return client.get(APP_HISTORY_PATH);
}
