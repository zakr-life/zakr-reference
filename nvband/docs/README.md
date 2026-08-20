# docs/

Cross-cutting documentation scaffolds required by CLAUDE.md §10, plus the
verbatim master build prompt this whole repository is built against.

- `ZAKR_NVBand_ClaudeCode_Master_Build_Prompt.md` — the full source
  specification (also summarized/linked from `../CLAUDE.md`).
- `risk-management/` — ISO 14971-style hazard log scaffold.
- `design-history-file/` — DHF folder scaffold, cross-referenced to
  where each artifact type actually lives in this repo.
- `open-items/` — mirrors CLAUDE.md §11's five open items; code
  references these via `TODO(OI-n)`.

Module-specific documentation (traceability matrix, CI gates, software
safety classification rationale) lives alongside the code it describes —
see `../firmware/docs/`.
