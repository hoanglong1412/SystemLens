/**
 * @file   sampleHelloApi.js
 * @brief  [SAMPLE API] Client for GET /api/v1/sample/hello (contract: api/openapi.yaml).
 *         Reference example for new api modules: one exported function per endpoint,
 *         taking an ApiClient and returning plain data.
 */

/** Path of the sample endpoint. */
export const SAMPLE_HELLO_PATH = '/api/v1/sample/hello';

/**
 * Fetch the greeting from the backend.
 * @param {import('../apiClient.js').ApiClient} client Configured API client.
 * @returns {Promise<string>} The greeting ("@@Hello world@@").
 * @throws {import('../apiClient.js').ApiError}
 */
export async function getSampleHello(client) {
  const body = await client.get(SAMPLE_HELLO_PATH);
  return body.message;
}
