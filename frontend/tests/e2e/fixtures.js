/**
 * @file   fixtures.js
 * @brief  Playwright fixtures of the Electron end-to-end tests:
 *          - electronApp: the SystemLens app (main process + real backend), recording a video;
 *          - window: its first BrowserWindow, once the shell is rendered;
 *          - api: calls the same backend as the app, with the app's token (to compare UI and data);
 *          - capture(name): full-window screenshot attached to the report and saved to the results.
 */
const path = require('node:path');
const fs = require('node:fs');
const { test: base, expect, _electron: electron } = require('@playwright/test');

/** Frontend root (the Electron app directory). */
const APP_DIR = path.join(__dirname, '..', '..');
/** Window content size (main.js) used for the video. */
const VIDEO_SIZE = { width: 1440, height: 960 };

/**
 * Electron's Chromium sandbox cannot start as root (containers, CI): disable it only there.
 * @returns {string[]} Extra command-line switches.
 */
function sandboxArgs() {
  return typeof process.getuid === 'function' && process.getuid() === 0 ? ['--no-sandbox'] : [];
}

const test = base.extend({
  /**
   * Launch the app with video recording; the video is attached to the test after the app closes.
   */
  electronApp: async ({}, use, testInfo) => {
    const videoDir = testInfo.outputPath('video');
    const app = await electron.launch({
      args: [APP_DIR, ...sandboxArgs()],
      cwd: APP_DIR,
      recordVideo: { dir: videoDir, size: VIDEO_SIZE },
    });
    await use(app);
    await app.close();
    // The video file is finalized when the app closes.
    for (const file of fs.existsSync(videoDir) ? fs.readdirSync(videoDir) : []) {
      await testInfo.attach('video', { path: path.join(videoDir, file), contentType: 'video/webm' });
    }
  },

  /**
   * First window of the app, after the renderer built the shell.
   */
  window: async ({ electronApp }, use) => {
    const page = await electronApp.firstWindow();
    page.on('console', (msg) => {
      if (msg.type() === 'error') console.log(`[renderer] ${msg.text()}`);
    });
    await page.waitForSelector('[data-testid="cpu-view"]');
    await use(page);
  },

  /**
   * GET a backend path with the app's own connection info (read through the preload bridge).
   */
  api: async ({ window }, use) => {
    const config = await window.evaluate(() => window.systemlens.getBackendConfig());
    const get = async (apiPath) => {
      const response = await fetch(config.baseUrl + apiPath, { headers: { Authorization: `Bearer ${config.token}` } });
      return { status: response.status, body: await response.json() };
    };
    await use({ get, config });
  },

  /**
   * Save a named screenshot of the window into the results and attach it to the report.
   */
  capture: async ({ window }, use, testInfo) => {
    let index = 0;
    await use(async (name) => {
      index += 1;
      const file = testInfo.outputPath(`${String(index).padStart(2, '0')}-${name}.png`);
      await window.screenshot({ path: file });
      await testInfo.attach(name, { path: file, contentType: 'image/png' });
    });
  },
});

module.exports = { test, expect };
