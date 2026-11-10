/**
 * @file   systemApi.js
 * @brief  Client for the "system" endpoints (contract: api/openapi.yaml).
 */

/** Path of getSystemOverview. */
export const SYSTEM_OVERVIEW_PATH = '/api/v1/system/overview';

/**
 * Fetch machine identity, uptime and quick stats (operationId getSystemOverview).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} SystemOverview object of the contract.
 * @throws {import('../apiClient.js').ApiError}
 */
export function getSystemOverview(client) {
  return client.get(SYSTEM_OVERVIEW_PATH);
}
