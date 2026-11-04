/**
 * @file   format.js
 * @brief  Text formatting of measured values for every screen (percent, clock, temperature, sizes,
 *         counts, durations, dates). A missing value (null / undefined) is always shown as PLACEHOLDER.
 */

/** Text shown for a value the platform cannot report. */
export const PLACEHOLDER = '—';

const MHZ_PER_GHZ = 1000;
const BYTES_PER_KB = 1024;
const SIZE_UNITS = ['B', 'KB', 'MB', 'GB', 'TB'];
const SECONDS_PER_MINUTE = 60;
const SECONDS_PER_HOUR = 3600;
const SECONDS_PER_DAY = 86400;

/**
 * @param {*} value
 * @returns {boolean} True when value is a finite number.
 */
export function isNumber(value) {
  return typeof value === 'number' && Number.isFinite(value);
}

/**
 * @param {number} n
 * @returns {string} n on two digits ("05").
 */
function pad2(n) {
  return String(n).padStart(2, '0');
}

/**
 * Percentage rounded to an integer.
 * @param {number|null} value 0..100.
 * @returns {string} e.g. "5%".
 */
export function formatPercent(value) {
  return isNumber(value) ? `${Math.round(value)}%` : PLACEHOLDER;
}

/**
 * Clock speed in GHz with two decimals.
 * @param {number|null} mhz Speed in MHz.
 * @returns {string} e.g. "2.61 GHz".
 */
export function formatGhz(mhz) {
  return isNumber(mhz) ? `${(mhz / MHZ_PER_GHZ).toFixed(2)} GHz` : PLACEHOLDER;
}

/**
 * Temperature rounded to an integer.
 * @param {number|null} celsius
 * @returns {string} e.g. "62 °C".
 */
export function formatCelsius(celsius) {
  return isNumber(celsius) ? `${Math.round(celsius)} °C` : PLACEHOLDER;
}

/**
 * Byte size with a binary unit and one decimal (KB below 1 MB without decimals).
 * @param {number|null} bytes
 * @returns {string} e.g. "111.1 MB", "256 KB".
 */
export function formatBytes(bytes) {
  if (!isNumber(bytes)) return PLACEHOLDER;
  let value = bytes;
  let unit = 0;
  while (value >= BYTES_PER_KB && unit < SIZE_UNITS.length - 1) {
    value /= BYTES_PER_KB;
    unit += 1;
  }
  const decimals = unit <= 1 ? 0 : 1;
  return `${value.toFixed(decimals)} ${SIZE_UNITS[unit]}`;
}

/**
 * Integer with thousands separators.
 * @param {number|null} value
 * @returns {string} e.g. "114,453".
 */
export function formatInteger(value) {
  return isNumber(value) ? Math.round(value).toLocaleString('en-US') : PLACEHOLDER;
}

/**
 * Duration as days:hours:minutes:seconds, the Task Manager "Up time" format.
 * @param {number|null} seconds
 * @returns {string} e.g. "45:01:06:32".
 */
export function formatUptime(seconds) {
  if (!isNumber(seconds)) return PLACEHOLDER;
  const total = Math.floor(seconds);
  const days = Math.floor(total / SECONDS_PER_DAY);
  const hours = Math.floor((total % SECONDS_PER_DAY) / SECONDS_PER_HOUR);
  const minutes = Math.floor((total % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE);
  return `${days}:${pad2(hours)}:${pad2(minutes)}:${pad2(total % SECONDS_PER_MINUTE)}`;
}

/**
 * Local date and time, day first (as in the design).
 * @param {string|null} iso RFC 3339 timestamp.
 * @param {boolean} [withSeconds]
 * @returns {string} e.g. "14.09.2026 08:12" or "14.09.2026 08:12:41".
 */
export function formatDateTime(iso, withSeconds = false) {
  const date = iso ? new Date(iso) : null;
  if (!date || Number.isNaN(date.getTime())) return PLACEHOLDER;
  const day = `${pad2(date.getDate())}.${pad2(date.getMonth() + 1)}.${date.getFullYear()}`;
  const time = `${pad2(date.getHours())}:${pad2(date.getMinutes())}`;
  return withSeconds ? `${day} ${time}:${pad2(date.getSeconds())}` : `${day} ${time}`;
}
