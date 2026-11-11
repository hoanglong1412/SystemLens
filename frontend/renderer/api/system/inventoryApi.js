/**
 * @file   inventoryApi.js
 * @brief  Client for the "inventory" endpoints: services, startup items and users (contract: api/openapi.yaml).
 */

/** Path of getServices. */
export const SERVICES_PATH = '/api/v1/services';
/** Path of getStartupItems. */
export const STARTUP_PATH = '/api/v1/startup';
/** Path of getUsers. */
export const USERS_PATH = '/api/v1/users';

/**
 * Fetch the installed services (operationId getServices).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} ServiceList object of the contract (available, reason, services).
 * @throws {import('../apiClient.js').ApiError}
 */
export function getServices(client) {
  return client.get(SERVICES_PATH);
}

/**
 * Fetch the startup items (operationId getStartupItems).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} StartupList object of the contract (available, reason, items).
 * @throws {import('../apiClient.js').ApiError}
 */
export function getStartupItems(client) {
  return client.get(STARTUP_PATH);
}

/**
 * Fetch the logon sessions and the usage per user (operationId getUsers).
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<object>} UserList object of the contract (available, reason, sessions, users).
 * @throws {import('../apiClient.js').ApiError}
 */
export function getUsers(client) {
  return client.get(USERS_PATH);
}
