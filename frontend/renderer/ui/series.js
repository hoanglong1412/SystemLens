/**
 * @file   series.js
 * @brief  Helpers that prepare history arrays from the API for plotting: padding to the common time
 *         axis, filling gaps and scaling raw rates to the peak of the window.
 */

/** Points per plot: 60 one-second intervals. Shorter histories are padded on the left. */
export const HISTORY_POINTS = 61;
/** Full scale of a percentage series. */
export const MAX_PERCENT = 100;

/**
 * Bring a history to exactly HISTORY_POINTS values so every plot has the same time axis.
 * @param {Array<number|null>|undefined} values Samples, oldest first.
 * @param {number|null} filler Value for the samples that do not exist yet.
 * @returns {Array<number|null>}
 */
export function padHistory(values, filler) {
  const recent = (values ?? []).slice(-HISTORY_POINTS);
  return [...new Array(HISTORY_POINTS - recent.length).fill(filler), ...recent];
}

/**
 * Prepare a history that may contain gaps (null) for plotting.
 * @param {Array<number|null>|undefined} values Samples, oldest first.
 * @returns {number[]|null} Padded series where each gap repeats the nearest known value;
 *          null when no sample is known (the series is then not drawn).
 */
export function gaplessHistory(values) {
  const padded = padHistory(values, null);
  const firstKnown = padded.find((value) => value !== null);
  if (firstKnown === undefined) return null;
  let last = firstKnown;
  return padded.map((value) => {
    if (value !== null) last = value;
    return Math.min(last, MAX_PERCENT);
  });
}

/**
 * Scale raw samples (bytes or bits per second) to percentages of the peak of all given series,
 * so related series (read / write, send / receive) share one scale.
 * @param {Array<Array<number>|undefined>} seriesList Raw histories, oldest first.
 * @param {number} minScale Smallest full scale in raw units, so an idle device does not amplify noise.
 * @returns {{scaled: number[][], peak: number}} Padded series in 0..100 and the raw value of 100%.
 */
export function scaleToPeak(seriesList, minScale) {
  const padded = seriesList.map((values) => padHistory(values, 0));
  const peak = Math.max(minScale, ...padded.flat());
  return { scaled: padded.map((values) => values.map((v) => (v / peak) * MAX_PERCENT)), peak };
}
