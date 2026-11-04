/**
 * @file   playwright.config.js
 * @brief  Playwright configuration of the Electron end-to-end tests (frontend/tests/e2e/).
 *
 * The app is launched by the fixture in tests/e2e/fixtures.js (Playwright's _electron API), with the
 * real Debug backend. Every test records a video and attaches named screenshots; results and the
 * HTML report go to tests/e2e/results/ and tests/e2e/report/ (both ignored by git).
 * On Linux run under a display server, e.g. `xvfb-run -a npm run test:e2e`.
 */
const { defineConfig } = require('@playwright/test');

module.exports = defineConfig({
  testDir: './tests/e2e',
  outputDir: './tests/e2e/results',
  // One Electron window + backend per test; running them in parallel would only fight for the CPU
  // that the tests measure.
  workers: 1,
  fullyParallel: false,
  timeout: 60_000,
  expect: { timeout: 10_000 },
  reporter: [['list'], ['html', { outputFolder: './tests/e2e/report', open: 'never' }]],
  use: {
    trace: 'retain-on-failure',
  },
});
