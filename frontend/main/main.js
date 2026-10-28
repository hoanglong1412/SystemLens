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

/** Content size of the window: the size of the Figma frame the UI is designed for. */
const WINDOW_WIDTH = 1440;
const WINDOW_HEIGHT = 960;
/** Height of the title bar drawn by the renderer (renderer/styles/shell.css). */
const TITLE_BAR_HEIGHT = 36;
const TITLE_BAR_COLOR = '#16171a';
const TITLE_BAR_SYMBOL_COLOR = '#8e9099';

/** Error from backend startup, reported to the renderer instead of a config. */
let backendStartError = null;

/**
 * Create the main application window.
 */
function createWindow() {
  const win = new BrowserWindow({
    width: WINDOW_WIDTH,
    height: WINDOW_HEIGHT,
    useContentSize: true,
    // The renderer draws its own title bar; the native minimize/maximize/close buttons are
    // overlaid on its right end so they keep working.
    titleBarStyle: 'hidden',
    titleBarOverlay: { color: TITLE_BAR_COLOR, symbolColor: TITLE_BAR_SYMBOL_COLOR, height: TITLE_BAR_HEIGHT },
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
