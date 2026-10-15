/**
 * @file   preload.js
 * @brief  Bridge between the Electron main process and the renderer (contextIsolation is on).
 *         Exposes window.systemlens with the minimum the renderer needs.
 */
const { contextBridge, ipcRenderer } = require('electron');

// Same value as BACKEND_GET_CONFIG in main/ipcChannels.js. Repeated on purpose: a sandboxed
// preload can only require('electron'), not local files.
const BACKEND_GET_CONFIG = 'backend:getConfig';

contextBridge.exposeInMainWorld('systemlens', {
  /**
   * Get the backend connection info.
   * @returns {Promise<{baseUrl: string, token: string}>}
   */
  getBackendConfig: () => ipcRenderer.invoke(BACKEND_GET_CONFIG),
});
