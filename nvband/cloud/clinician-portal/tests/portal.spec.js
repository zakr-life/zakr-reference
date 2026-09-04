// @ts-check
import { test } from '@playwright/test';

// The clinician portal (CLAUDE.md §6) has no web UI yet — rbac.js and
// sleepTrend.js are backend logic only, covered by the node:test suite in
// ../tests (nvband/cloud/tests). Add Playwright specs here once a portal
// server/UI exists; point playwright.config.js's `use.baseURL` (and an
// eventual `webServer` block) at it.
test.skip('clinician portal UI', () => {});
