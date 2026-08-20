# docs/risk-management/

ISO 14971-style hazard log scaffold (CLAUDE.md §10).

`hazard_log.csv` is seeded with the hazards named explicitly in the
CLAUDE.md dossier: net-DC injury, over-current from a lifted electrode,
interlock defeat, undetected processor failure, OTA-introduced
regression, and data confidentiality breach — plus two this codebase's
own construction surfaced (motion-contaminated-epoch misclassification,
and a manufacturing gate being skipped/falsified).

Every row's `Likelihood`, `Severity`, `Risk_Level`, and
`Reviewer_Signoff` columns are placeholders. **This document does not
perform risk assessment** — that requires a qualified risk reviewer
applying ZAKR's actual risk acceptability criteria, which do not exist
in this repository. What this document does provide: every hazard is
already linked to the specific code module that mitigates it and the
specific automated test that verifies that mitigation runs and passes
today (see the `Mitigation_Code_Reference` / `Mitigation_Test_Reference`
columns, cross-referenced to `firmware/docs/traceability_matrix.csv`).

This is not a substitute for a real ISO 14971 risk management file. It
is a starting scaffold structured so a qualified reviewer's work
(likelihood/severity scoring, residual risk acceptability, risk-benefit
analysis) has somewhere concrete to attach to, rather than starting from
a blank page or from prose scattered across design documents.
