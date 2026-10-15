/**
 * @file   main.js
 * @brief  Electron main process: starts the C++ backend, exposes its connection info over IPC
 *         and opens the application window.
 */
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('node:path');
const { BackendProcess } = require('./backendProcess');
const { BACKEND_GET_CONFIG } = require('./ipcChannels');

const backend = new BackendProcess();

/** Error from backend startup, reported to the renderer instead of a config. */
let backendStartError = null;

/**
 * Create the main application window.
 */
function createWindow() {
  const win = new BrowserWindow({
    width: 1000,
    height: 700,
    webPreferences: {
      preload: path.join(__dirname, '..', 'preload', 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
    },
  });

  win.loadFile(path.join(__dirname, '..', 'renderer', 'index.html'));
}

/**
 * Answer the renderer's request for backend connection info.
 * Throwing here rejects the renderer's promise, which shows the error in the UI.
 * @returns {{baseUrl: string, token: string}}
 */
function handleGetBackendConfig() {
  if (backendStartError) {
    throw backendStartError;
  }
  return backend.getConfig();
}

app.whenReady().then(async () => {
  try {
    await backend.start();
  } catch (err) {
    // Keep going: the window still opens and displays the error.
    backendStartError = err;
    console.error(err);
  }

  ipcMain.handle(BACKEND_GET_CONFIG, handleGetBackendConfig);
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

app.on('will-quit', () => backend.stop());
