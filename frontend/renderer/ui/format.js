/**
 * @file   format.js
 * @brief  Turns raw API numbers (percent, MHz, bytes, bits per second, seconds, timestamps) into the
 *         texts the UI shows. The only place where units and number formats are decided.
 */

/** Shown wherever a value is not available (null from the API, or no data yet). */
export const NO_VALUE = '—';

const BYTES_PER_KIB = 1024;
const BYTES_PER_MIB = 1024 ** 2;
const BYTES_PER_GIB = 1024 ** 3;
const BYTES_PER_TIB = 1024 ** 4;
const MHZ_PER_GHZ = 1000;
const BITS_PER_KBIT = 1000;
const BITS_PER_MBIT = 1000 ** 2;
const SECONDS_PER_MINUTE = 60;
const SECONDS_PER_HOUR = 3600;
const SECONDS_PER_DAY = 86400;
const NUMBER_LOCALE = 'en-US';

/**
 * @param {number|null|undefined} value
 * @returns {boolean} True when the value can be formatted as a number.
 */
function isNumber(value) {
  return typeof value === 'number' && Number.isFinite(value);
}

/**
 * @param {number} value
 * @returns {string} Two digits with a leading zero ("07").
 */
function twoDigits(value) {
  return String(value).padStart(2, '0');
}

/**
 * @param {number|null} value Percentage (0..100).
 * @param {number} [digits] Decimal places.
 * @returns {string} e.g. "5%", "3.1%".
 */
export function formatPercent(value, digits = 0) {
  return isNumber(value) ? `${value.toFixed(digits)}%` : NO_VALUE;
}

/**
 * @param {number|null} mhz Clock in MHz.
 * @returns {string} e.g. "2.61 GHz".
 */
export function formatFrequency(mhz) {
  return isNumber(mhz) ? `${(mhz / MHZ_PER_GHZ).toFixed(2)} GHz` : NO_VALUE;
}

/**
 * @param {number|null} celsius
 * @returns {string} e.g. "62 °C".
 */
export function formatTemperature(celsius) {
  return isNumber(celsius) ? `${Math.round(celsius)} °C` : NO_VALUE;
}

/**
 * @param {number|null} value
 * @returns {string} Rounded, with thousands separators, e.g. "114,453".
 */
export function formatInteger(value) {
  return isNumber(value) ? Math.round(value).toLocaleString(NUMBER_LOCALE) : NO_VALUE;
}

/**
 * @param {number|null} bytes
 * @param {number} [digits] Decimal places.
 * @returns {string} Mebibytes, e.g. "612 MB", "111.1 MB".
 */
export function formatMegabytes(bytes, digits = 0) {
  return isNumber(bytes) ? `${(bytes / BYTES_PER_MIB).toFixed(digits)} MB` : NO_VALUE;
}

/**
 * @param {number|null} bytes
 * @returns {string} Gibibytes without unit, one decimal, e.g. "6.9".
 */
export function formatGigabytesValue(bytes) {
  return isNumber(bytes) ? (bytes / BYTES_PER_GIB).toFixed(1) : NO_VALUE;
}

/**
 * Cache sizes: whole kibibytes below 1 MiB, otherwise mebibytes with one decimal.
 * @param {number|null} bytes
 * @returns {string} e.g. "256 KB", "8.0 MB".
 */
export function formatCacheSize(bytes) {
  if (!isNumber(bytes)) return NO_VALUE;
  return bytes < BYTES_PER_MIB
    ? `${Math.round(bytes / BYTES_PER_KIB)} KB`
    : `${(bytes / BYTES_PER_MIB).toFixed(1)} MB`;
}

/**
 * @param {number|null} bitsPerSecond
 * @returns {string} e.g. "184 Kbps", "12.4 Mbps".
 */
export function formatBitRate(bitsPerSecond) {
  if (!isNumber(bitsPerSecond)) return NO_VALUE;
  return bitsPerSecond < BITS_PER_MBIT
    ? `${Math.round(bitsPerSecond / BITS_PER_KBIT)} Kbps`
    : `${(bitsPerSecond / BITS_PER_MBIT).toFixed(1)} Mbps`;
}

/**
 * @param {number|null} seconds Uptime.
 * @returns {string} "days:hours:minutes:seconds", e.g. "45:01:06:32".
 */
export function formatUptime(seconds) {
  if (!isNumber(seconds)) return NO_VALUE;
  const total = Math.max(0, Math.floor(seconds));
  const days = Math.floor(total / SECONDS_PER_DAY);
  const hours = Math.floor((total % SECONDS_PER_DAY) / SECONDS_PER_HOUR);
  const minutes = Math.floor((total % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE);
  return `${days}:${twoDigits(hours)}:${twoDigits(minutes)}:${twoDigits(total % SECONDS_PER_MINUTE)}`;
}

/**
 * @param {string|null} isoTime RFC 3339 timestamp from the API.
 * @param {object} [options]
 * @param {boolean} [options.seconds] Include seconds.
 * @returns {string} Local time as "DD.MM.YYYY HH:MM[:SS]", e.g. "14.09.2026 08:12:41".
 */
export function formatDateTime(isoTime, { seconds = false } = {}) {
  const date = new Date(isoTime ?? NaN);
  if (Number.isNaN(date.getTime())) return NO_VALUE;
  const day = `${twoDigits(date.getDate())}.${twoDigits(date.getMonth() + 1)}.${date.getFullYear()}`;
  const time = `${twoDigits(date.getHours())}:${twoDigits(date.getMinutes())}`;
  return seconds ? `${day} ${time}:${twoDigits(date.getSeconds())}` : `${day} ${time}`;
}

/**
 * @param {number} count
 * @param {string} singular Noun for exactly one.
 * @param {string} plural Noun otherwise.
 * @returns {string} e.g. "1 socket", "8 logical processors".
 */
export function formatCount(count, singular, plural) {
  return isNumber(count) ? `${count} ${count === 1 ? singular : plural}` : NO_VALUE;
}

/**
 * Pick the unit that keeps the number short.
 * @param {number|null} bytes
 * @param {number} [digits] Decimal places for GB and above (smaller units are rounded).
 * @returns {string} e.g. "812 B", "34 KB", "6.9 GB", "2.0 TB".
 */
export function formatBytes(bytes, digits = 1) {
  if (!isNumber(bytes)) return NO_VALUE;
  if (bytes < BYTES_PER_KIB) return `${Math.round(bytes)} B`;
  if (bytes < BYTES_PER_MIB) return `${Math.round(bytes / BYTES_PER_KIB)} KB`;
  if (bytes < BYTES_PER_GIB) return `${(bytes / BYTES_PER_MIB).toFixed(bytes < 10 * BYTES_PER_MIB ? digits : 0)} MB`;
  if (bytes < BYTES_PER_TIB) return `${(bytes / BYTES_PER_GIB).toFixed(digits)} GB`;
  return `${(bytes / BYTES_PER_TIB).toFixed(digits)} TB`;
}

/**
 * @param {number|null} bytesPerSecond
 * @returns {string} e.g. "1.2 MB/s".
 */
export function formatByteRate(bytesPerSecond) {
  return isNumber(bytesPerSecond) ? `${formatBytes(bytesPerSecond)}/s` : NO_VALUE;
}

/**
 * @param {number|null} seconds Accumulated CPU time.
 * @returns {string} "hours:minutes:seconds", e.g. "1:02:03".
 */
export function formatCpuTime(seconds) {
  if (!isNumber(seconds)) return NO_VALUE;
  const total = Math.max(0, Math.floor(seconds));
  const hours = Math.floor(total / SECONDS_PER_HOUR);
  const minutes = Math.floor((total % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE);
  return `${hours}:${twoDigits(minutes)}:${twoDigits(total % SECONDS_PER_MINUTE)}`;
}
