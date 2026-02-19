# BlueprintSerializer Agent Guide

This file is the first-stop context for any AI agent working inside `Plugins/BlueprintSerializer`.

## Canonical References

Always read these before making changes:

- `FULL_EXTRACTION_DEFINITION.md`
- `CONVERTER_READY_EXTRACTION_SPEC.md`
- `CONVERTER_READY_TODO.md`
- `REGRESSION_HARNESS.md`
- `REGRESSION_LOG.md`
- `REGRESSION_BASELINE.json`

## Required Regression Workflow

For any non-trivial change, run this sequence:

1. Build plugin/editor target.
2. Run `BP_SLZR.RunRegressionSuite`.
3. Verify results from artifact files/logs, not shell timeout status.
4. Update `CONVERTER_READY_TODO.md` with evidence-based metrics.
5. Append a new entry to `REGRESSION_LOG.md`.

## Command Defaults

- Preferred host-side entrypoint (auto timeout + auto-close UE process):
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -ExportDir Saved/BlueprintExports/BP_SLZR_All_<timestamp> -SkipExport -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`

- Full run (new export dir):
  - `BP_SLZR.RunRegressionSuite`
- Reuse existing batch (faster validation-only path):
  - `BP_SLZR.RunRegressionSuite Saved/BlueprintExports/BP_SLZR_All_<timestamp> true`
- Override baseline file (optional):
  - `BP_SLZR.RunRegressionSuite Saved/BlueprintExports/BP_SLZR_All_<timestamp> true Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`

The suite runs:

- `BP_SLZR.ExportAllBlueprints` (unless `skipExport=true`)
- `BP_SLZR.ValidateConverterReady`
- `BP_SLZR.AuditAnimationCurves`

## Anti-Regression Rules

- Do not claim success unless artifact files exist and contain expected metrics.
- Require `suitePass=true` in `BP_SLZR_RegressionRun_<timestamp>.json` before marking a regression run successful.
- Treat Unreal command timeouts as inconclusive; confirm completion via `Saved/Logs/LyraStarterGame.log` and report JSON files.
- Keep status language precise:
  - `done`: implemented and validated with corpus evidence.
  - `in_progress`: implemented but validation incomplete.
  - `blocked`: validation blocked by corpus/tooling constraints.

## Scope Priority

Prioritize work that AI can reliably execute and validate without manual editor authoring.
Animation content authoring/visual tuning stays human-led; automation should focus on extraction fidelity, validation gates, and deterministic reporting.

Persistent mandate:
- Primary: C++ conversion fidelity for gameplay/class logic (`CR-035`, `CR-036`, `CR-006`, then remaining CXX-* mapped items).
- Secondary: editor-heavy animation authoring/deep animation polish unless explicitly requested.
- Keep `CONVERTER_READY_TODO.md` in normalized backlog mode (`todo` rows with "left off" context) so new AI sessions inherit the same priority framing.
