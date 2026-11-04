/**
 * @file   systemApi.js
 * @brief  Client for GET /api/v1/system/info (contract: api/openapi.yaml, schema SystemInfo).
 */

/** Path of the machine identity. */
export const SYSTEM_INFO_PATH = '/api/v1/system/info';

/**
 * Fetch the identity of the machine the backend runs on.
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<{hostName: string, osName: string, osVersion: string, userName: string, platform: string}>}
 * @throws {import('../apiClient.js').ApiError} 501 when the platform has no system info provider.
 */
export function getSystemInfo(client) {
  return client.get(SYSTEM_INFO_PATH);
}
