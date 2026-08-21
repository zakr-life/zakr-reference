/**
 * voiceprintTemplateStore.js — Opt-in-gated, template-shape-verified
 * storage policy for voiceprint templates.
 *
 * Addendum 2 §B: "[raw audio is] never transmitted, unless the user has
 * separately and explicitly opted into cloud voiceprint backup (a
 * distinct, off-by-default consent toggle) — and even then, only the
 * fixed-length embedding vector may sync, never raw audio."
 *
 * `TODO(OI-9)`: voiceprint (and brainprint) templates are biometric
 * data; jurisdictional biometric-privacy statutes (e.g. Illinois BIPA
 * and similar) impose specific notice, consent, and retention/
 * destruction obligations. This module implements the technical
 * *capability* (explicit consent gate, defined shape/size limits,
 * refusal by default) but is not itself a legal compliance review —
 * flagged for counsel review before this ships, exactly as OI-2/OI-5
 * flag hardware items pending a qualified reviewer.
 *
 * Policy logic ONLY -- no HTTP framework, matching every other module in
 * cloud/ (see cloud/README.md, "Not included in this pass").
 */

/* Must match models/voiceprint/synthetic_voice.py's FEATURE_DIM exactly
 * -- that is the only shape a real enrollment template ever has. */
const EXPECTED_TEMPLATE_LENGTH = 16;

/* A generous ceiling, still far below any realistic raw-audio buffer
 * (even a fraction of a second of PCM/PDM-decimated audio is thousands
 * of samples) -- used only as a structural, shape-based rejection of
 * "this looks like audio, not a template," independent of the exact
 * expected length check below. */
const MAX_PLAUSIBLE_TEMPLATE_LENGTH = 64;

/**
 * Structural check: does this payload even have the SHAPE of a
 * fixed-length numeric feature vector, as opposed to a raw audio sample
 * buffer? Deliberately conservative -- anything that isn't a plain
 * array of finite numbers within a short, template-plausible length is
 * rejected here, before any consent/length-match logic runs.
 */
function looksLikeTemplate(payload) {
  if (!Array.isArray(payload)) {
    // Rejects typed arrays (e.g. Int16Array, the shape a raw PDM/PCM
    // buffer would actually arrive as), objects, strings, buffers, etc.
    return false;
  }
  if (payload.length === 0 || payload.length > MAX_PLAUSIBLE_TEMPLATE_LENGTH) {
    return false;
  }
  return payload.every((v) => typeof v === 'number' && Number.isFinite(v));
}

/**
 * @param {{voiceprintCloudBackupConsent: boolean}} consent
 * @param {number[]} template
 * @returns {{stored: object|null, reason: string}} stored is null if
 *   consent is not granted OR the payload is not template-shaped -- this
 *   function refuses to even attempt storage rather than store-then-
 *   discard, mirroring cloud/analytics/deidentify.js's
 *   deidentifyForAnalytics() "refuse to even attempt" pattern. There is
 *   no code path here where a non-consenting user's payload, or a
 *   raw-audio-shaped payload, transiently exists in a "to be stored"
 *   object.
 */
function storeVoiceprintTemplate(consent, template) {
  if (!consent || consent.voiceprintCloudBackupConsent !== true) {
    return { stored: null, reason: 'no voiceprint-cloud-backup consent on file' };
  }

  if (!looksLikeTemplate(template)) {
    return {
      stored: null,
      reason: 'payload is not template-shaped (expected a short, fixed-length numeric ' +
        'vector, not raw audio)',
    };
  }

  if (template.length !== EXPECTED_TEMPLATE_LENGTH) {
    return {
      stored: null,
      reason: `template length ${template.length} does not match expected length ` +
        `${EXPECTED_TEMPLATE_LENGTH}`,
    };
  }

  return {
    stored: {
      template: [...template], // copy, never a reference to caller-owned data
      storedAt: Date.now(),
    },
    reason: 'ok',
  };
}

module.exports = {
  storeVoiceprintTemplate,
  looksLikeTemplate,
  EXPECTED_TEMPLATE_LENGTH,
  MAX_PLAUSIBLE_TEMPLATE_LENGTH,
};
