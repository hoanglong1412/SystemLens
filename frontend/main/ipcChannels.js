/**
 * @file   ipcChannels.js
 * @brief  Names of the IPC channels between the Electron main process and the preload script.
 *
 * Keep in sync with frontend/preload/preload.js: the sandboxed preload cannot
 * require() local files, so it repeats these strings.
 */

/** Channel used by the renderer (via preload) to get { baseUrl, token } of the backend. */
const BACKEND_GET_CONFIG = 'backend:getConfig';

module.exports = { BACKEND_GET_CONFIG };
