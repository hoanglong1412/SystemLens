/**
 * @file   cpuPerformance.spec.js
 * @brief  End-to-end tests of the Performance > CPU screen (spec cpu-performance-view): the Electron
 *         app runs with the real backend, values on screen are compared with the backend API, and
 *         each step is captured (screenshots + video, see fixtures.js).
 */
const { spawn } = require('node:child_process');
const { test, expect } = require('./fixtures');

/** Placeholder shown for a missing value (renderer/ui/format.js). */
const PLACEHOLDER = '—';
/** Longer than one polling period + one sampling interval. */
const REFRESH_WAIT_MS = 3000;
/** How long the walkthrough lets the charts fill before the first capture. */
const WALKTHROUGH_FILL_MS = 15000;
/** How long the CPU load generator runs. */
const LOAD_DURATION_MS = 8000;
/** Lower bound of total usage with one busy process (100 / N % on N logical processors, minus noise). */
const MIN_USAGE_UNDER_LOAD = 10;
/** Extra time under load before the capture, so the 60 s charts show the rise. */
const LOAD_VISIBLE_MS = 6000;

/**
 * Locator of a summary / scheduler metric value by its label.
 * @param {import('@playwright/test').Page} window
 * @param {string} label e.g. "Utilization".
 * @returns {import('@playwright/test').Locator}
 */
function metricValue(window, label) {
  return window.locator(`[data-metric="${label}"] .metric__value`).first();
}

/**
 * Wait until the first metrics response has been rendered.
 * @param {import('@playwright/test').Page} window
 */
async function waitForLiveData(window) {
  await expect(metricValue(window, 'Utilization')).not.toHaveText(PLACEHOLDER);
  await expect(window.getByTestId('uptime')).not.toHaveText(PLACEHOLDER);
}

test.describe('Performance > CPU', () => {
  test('opens on the CPU screen with the processor facts of the backend', async ({ window, api, capture }) => {
    await capture('startup');
    await waitForLiveData(window);

    const info = await api.get('/api/v1/cpu/info');
    expect(info.status).toBe(200);
    await expect(window.getByTestId('cpu-model')).toHaveText(info.body.model);
    await expect(window.getByTestId('cpu-topology')).toContainText(`${info.body.logicalProcessors} logical processor`);
    await expect(window.locator('[data-spec="Logical processors"] .spec__value'))
      .toHaveText(String(info.body.logicalProcessors));
    await expect(window.locator('[data-spec="Cores"] .spec__value')).toHaveText(String(info.body.cores));
    await expect(window.locator('[data-spec="Sockets"] .spec__value')).toHaveText(String(info.body.sockets));
    await capture('cpu-overview-live');
  });

  test('summary values match the metrics endpoint', async ({ window, api }) => {
    await waitForLiveData(window);
    const metrics = await api.get('/api/v1/cpu/metrics');
    expect(metrics.status).toBe(200);
    const { current, history, historyLength } = metrics.body;

    expect(current.usagePercent).toBeGreaterThanOrEqual(0);
    expect(current.usagePercent).toBeLessThanOrEqual(100);
    expect(history.usagePercent.length).toBeLessThanOrEqual(historyLength);

    await expect(metricValue(window, 'Utilization')).toHaveText(/^\d{1,3}%$/);
    // Process counts move between two samples: compare within a small margin.
    const shown = Number((await metricValue(window, 'Processes').textContent()).replace(/,/g, ''));
    expect(Math.abs(shown - current.processes)).toBeLessThanOrEqual(10);
    // The status bar reports a healthy connection.
    await expect(window.getByTestId('status-connection')).toContainText('Connected to backend');
  });

  test('refreshes every second', async ({ window, capture }) => {
    await waitForLiveData(window);
    const uptime = window.getByTestId('uptime');
    const before = await uptime.textContent();
    await window.waitForTimeout(REFRESH_WAIT_MS);
    await expect(uptime).not.toHaveText(before);
    await expect(window.getByTestId('chip-uptime')).toContainText('Up ');
    await capture('after-refresh');
  });

  test('header shows the machine identity', async ({ window, api, capture }) => {
    const system = await api.get('/api/v1/system/info');
    expect(system.status).toBe(200);
    await expect(window.getByTestId('machine-name')).toContainText(system.body.hostName);
    await expect(window.getByTestId('machine-name')).toContainText(system.body.osName);
    await expect(window.getByTestId('machine-details')).toContainText(system.body.osVersion);
    await capture('header');
  });

  test('top processes table is sorted by CPU and limited to 10 rows', async ({ window, capture }) => {
    await waitForLiveData(window);
    const rows = window.getByTestId('top-processes').locator('tbody tr');
    await expect(rows.first()).toBeVisible();
    const count = await rows.count();
    expect(count).toBeGreaterThan(0);
    expect(count).toBeLessThanOrEqual(10);

    const cpu = (await window.getByTestId('top-processes').locator('td.proc__cpu').allTextContents()).map(Number);
    for (let i = 1; i < cpu.length; i++) expect(cpu[i - 1]).toBeGreaterThanOrEqual(cpu[i]);
    await window.getByTestId('card-processes').screenshot({ path: test.info().outputPath('top-processes.png') });
    await capture('top-processes');
  });

  test('Per-core sub-tab shows one chart per logical processor', async ({ window, api, capture }) => {
    await waitForLiveData(window);
    const metrics = await api.get('/api/v1/cpu/metrics');
    const cores = metrics.body.current.perCoreUsagePercent.length;

    await window.getByTestId('subtab-Per-core').click();
    await expect(window.getByTestId('subtab-Per-core')).toHaveClass(/subtab--active/);
    await expect(window.getByTestId('core-grid')).toBeVisible();
    await expect(window.getByTestId('core-tile')).toHaveCount(cores);
    await expect(window.getByTestId('card-utilization')).toBeHidden();
    await window.waitForTimeout(REFRESH_WAIT_MS); // let the per-core charts collect a few samples
    await capture('per-core');

    await window.getByTestId('subtab-Overview').click();
    await expect(window.getByTestId('card-utilization')).toBeVisible();
    await expect(window.getByTestId('core-grid')).toBeHidden();
  });

  test('tooltip stays open across a data refresh', async ({ window, capture }) => {
    await waitForLiveData(window);
    await window.getByTestId('card-utilization').locator('.info-icon').hover();
    const tooltip = window.locator('.tooltip');
    await expect(tooltip).toHaveClass(/tooltip--visible/);
    await expect(tooltip).toContainText('% Utilization');
    await window.waitForTimeout(REFRESH_WAIT_MS);
    await expect(tooltip).toHaveClass(/tooltip--visible/);
    await capture('tooltip');
  });

  test('usage rises under CPU load', async ({ window, capture }) => {
    await waitForLiveData(window);
    // One busy process: on N logical processors total usage rises by about 100 / N %.
    const load = spawn(process.execPath, ['-e', 'for(;;){}'], { stdio: 'ignore' });
    try {
      const usage = metricValue(window, 'Utilization');
      await expect.poll(async () => Number((await usage.textContent()).replace('%', '')), {
        timeout: LOAD_DURATION_MS,
      }).toBeGreaterThanOrEqual(MIN_USAGE_UNDER_LOAD);
      // Keep the load a few more seconds so the charts show the rise, then capture.
      await window.waitForTimeout(LOAD_VISIBLE_MS);
      await capture('under-load');
      // The busy process shows up in the top processes table.
      await expect(window.getByTestId('top-processes').locator('td.proc__pid', { hasText: String(load.pid) }))
        .toHaveCount(1);
    } finally {
      load.kill();
    }
  });

  test('toggle switches to the hello screen and back', async ({ window, api, capture }) => {
    const hello = await api.get('/api/v1/sample/hello');
    await window.getByTestId('view-toggle').click();
    await expect(window.locator('#message')).toHaveText(hello.body.message);
    await capture('hello');
    await window.getByTestId('view-toggle').click();
    await expect(window.getByTestId('cpu-view')).toBeVisible();
  });

  test('walkthrough: live charts, tooltip, per-core, hello', async ({ window, capture }) => {
    // One continuous session for the recorded video: let the 60 s charts fill for a while.
    await waitForLiveData(window);
    await window.waitForTimeout(WALKTHROUGH_FILL_MS);
    await capture('walkthrough-overview');

    await window.getByTestId('card-breakdown').locator('.info-icon').hover();
    await expect(window.locator('.tooltip')).toHaveClass(/tooltip--visible/);
    await capture('walkthrough-tooltip');
    await window.mouse.move(0, 0);

    await window.getByTestId('subtab-Per-core').click();
    await window.waitForTimeout(REFRESH_WAIT_MS);
    await capture('walkthrough-per-core');
    await window.getByTestId('subtab-Overview').click();

    await window.getByTestId('view-toggle').click();
    await expect(window.locator('#message')).toBeVisible();
    await window.waitForTimeout(1000);
    await window.getByTestId('view-toggle').click();
    await expect(window.getByTestId('cpu-view')).toBeVisible();
    await window.waitForTimeout(REFRESH_WAIT_MS);
    await capture('walkthrough-back-to-cpu');
  });
});
