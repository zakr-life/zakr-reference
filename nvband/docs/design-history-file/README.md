# docs/design-history-file/

Design History File (DHF) folder scaffold (CLAUDE.md §10): "folder
structure only, cross-referenced to where each artifact type will live
once produced, not populated with unverified claims."

A DHF is not written by generating placeholder documents — it's compiled
from evidence a real design-control process produces over time. This
scaffold exists so that process has a place to file things, and so this
repository's own artifacts are cross-referenced from day one rather than
scattered.

| DHF section | Where the real artifact lives (once produced) |
|---|---|
| Design & development plan | Not present in this repo — a program-management artifact, not code. |
| Design inputs (requirements) | `../../CLAUDE.md` is the current design-intent source; a formal design-input document derived from it does not yet exist. |
| Design outputs | This repository's source code, organized per `../../README.md`'s directory map. |
| Verification | `firmware/docs/traceability_matrix.csv` (requirement -> code -> test), `firmware/docs/ci_gates.md`, `models/export/model_card.md`, and every `*/tests/` directory's passing results. |
| Validation | **Not present.** CLAUDE.md §9's explicit non-goal: none of this repository's testing constitutes IEC 60601-1 electrical safety testing, biocompatibility testing, or clinical validation. |
| Risk management file | `../risk-management/` (scaffold only — see its README). |
| Design reviews | Not present — a process artifact, not code. |
| Design changes | This repository's git history, once real design-control change orders exist to reference. |
| Regulatory correspondence | Not present. |

Per CLAUDE.md §11 OI-5: "Nothing here is a fabrication release or has
passed independent check." This scaffold applies that same discipline to
the DHF folder structure itself.
