/**
 * @file   apiClient.js
 * @brief  Generic HTTP client for the SystemLens backend API. Every api module
 *         (renderer/api/**) calls the backend through this class, never through fetch directly.
 *
 * Adds the bearer token, applies a timeout and turns error responses
 * (RFC 7807 application/problem+json, see docs/api.md) into ApiError.
 */

const DEFAULT_TIMEOUT_MS = 5000;
const PROBLEM_CONTENT_TYPE = 'application/problem+json';

/** Error raised for any failed API call (HTTP error, network failure or timeout). */
export class ApiError extends Error {
  /**
   * @param {string} message Human-readable description.
   * @param {object} [details]
   * @param {number} [details.status] HTTP status; 0 when no response was received.
   * @param {string} [details.title] Problem title from the backend, if any.
   */
  constructor(message, { status = 0, title = '' } = {}) {
    super(message);
    this.name = 'ApiError';
    this.status = status;
    this.title = title;
  }
}

export class ApiClient {
  /**
   * @param {object} config
   * @param {string} config.baseUrl Backend origin, e.g. "http://127.0.0.1:53121".
   * @param {string} config.token Bearer token for this run.
   * @param {number} [config.timeoutMs] Per-request timeout.
   */
  constructor({ baseUrl, token, timeoutMs = DEFAULT_TIMEOUT_MS }) {
    this.baseUrl = baseUrl;
    this.token = token;
    this.timeoutMs = timeoutMs;
  }

  /**
   * Send a GET request.
   * @param {string} path API path, e.g. "/api/v1/sample/hello".
   * @returns {Promise<any>} Parsed JSON body.
   * @throws {ApiError}
   */
  get(path) {
    return this.#request('GET', path);
  }

  /**
   * Send a request and parse the JSON response.
   * @param {string} method HTTP method.
   * @param {string} path API path.
   * @param {any} [body] Value sent as JSON.
   * @returns {Promise<any>} Parsed JSON body.
   * @throws {ApiError}
   */
  async #request(method, path, body) {
    const headers = { Authorization: `Bearer ${this.token}` };
    if (body !== undefined) {
      headers['Content-Type'] = 'application/json';
    }

    let response;
    try {
      response = await fetch(this.baseUrl + path, {
        method,
        headers,
        body: body === undefined ? undefined : JSON.stringify(body),
        signal: AbortSignal.timeout(this.timeoutMs),
      });
    } catch (err) {
      // fetch only rejects when no HTTP response arrived (backend down, timeout...).
      throw new ApiError(`${method} ${path} failed: ${err.message}`);
    }

    if (!response.ok) {
      throw await ApiClient.#toApiError(method, path, response);
    }
    return response.json();
  }

  /**
   * Build an ApiError from an HTTP error response, using the problem body when present.
   * @param {string} method HTTP method of the request.
   * @param {string} path Requested path.
   * @param {Response} response Error response.
   * @returns {Promise<ApiError>}
   */
  static async #toApiError(method, path, response) {
    const contentType = response.headers.get('Content-Type') || '';
    if (contentType.startsWith(PROBLEM_CONTENT_TYPE)) {
      const problem = await response.json();
      return new ApiError(`${problem.title}: ${problem.detail}`, { status: response.status, title: problem.title });
    }
    return new ApiError(`${method} ${path} returned HTTP ${response.status}`, { status: response.status });
  }
}
