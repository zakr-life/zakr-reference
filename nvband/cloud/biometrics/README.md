# cloud/biometrics/

Implements Addendum 2 §B's cloud-side rule: raw audio is never
transmitted, and even the fixed-length voiceprint TEMPLATE may only sync
if the user has separately opted into cloud voiceprint backup (a
distinct, off-by-default consent — see
`app/mobile/src/state/voiceprintConsent.js`'s
`voiceprintCloudBackupConsent`, independent of
`voiceprintEnrollmentConsent`).

Policy logic ONLY, no HTTP framework — matching the rest of `cloud/` (see
`cloud/README.md`, "Not included in this pass").

## Layout

- `voiceprintTemplateStore.js` — `storeVoiceprintTemplate(consent,
  template)` refuses to even attempt storage (returns `{stored: null,
  reason}`, matching `cloud/analytics/deidentify.js`'s
  `deidentifyForAnalytics()` "refuse to even attempt" pattern) unless
  BOTH: (a) `voiceprintCloudBackupConsent` is `true`, and (b) the payload
  is verified to be template-shaped — a short, fixed-length array of
  finite numbers matching `models/voiceprint/synthetic_voice.py`'s
  `FEATURE_DIM` — never something raw-audio-buffer-shaped (a typed array,
  an object, a too-long array, non-numeric entries).

`TODO(OI-9)`: voiceprint (and brainprint) templates are biometric data;
jurisdictional biometric-privacy statutes (e.g. Illinois BIPA and
similar) impose specific notice, consent, and retention/destruction
obligations. This directory implements the technical *capability*
(explicit consent gate, template-shape verification, refuse-by-default)
but is not itself a legal compliance review — flagged for counsel review
before this ships, exactly as OI-2/OI-5 flag hardware items pending a
qualified reviewer.

## What's not included in this pass

Retention-policy wiring (this template store has no expiry/deletion
logic of its own yet — `cloud/storage/retentionPolicy.js`'s existing "no
indefinite default" discipline is the pattern to apply once this module
is wired behind real storage), and HTTP framework wiring, for the same
reasons given in `cloud/README.md`.
