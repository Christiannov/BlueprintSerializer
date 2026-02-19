# BlueprintSerializer Regression Harness

Purpose: run repeatable, evidence-based regression checks so we do not re-learn the same failures.

Priority mandate:
- Use this harness primarily to advance AI-executable C++ conversion parity work.
- Treat editor-heavy animation authoring tasks as secondary unless explicitly requested.

## Preferred Launcher (Host Side)

Use the wrapper script when launching from terminal/automation. It enforces timeout, watches for suite artifacts, and closes lingering `UnrealEditor-Cmd` automatically:

- `Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1`

Example:

- `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -ExportDir Saved/BlueprintExports/BP_SLZR_All_20260218_5 -SkipExport -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`

## Single Entry Command

Use this command as the default harness entrypoint:

- `BP_SLZR.RunRegressionSuite [ExportDir] [SkipExport] [BaselinePath]`

Examples:

- Full suite with fresh export directory:
  - `BP_SLZR.RunRegressionSuite`
- Reuse existing export batch and skip export:
  - `BP_SLZR.RunRegressionSuite Saved/BlueprintExports/BP_SLZR_All_20260218_5 true`
- Reuse existing batch with explicit baseline file:
  - `BP_SLZR.RunRegressionSuite Saved/BlueprintExports/BP_SLZR_All_20260218_5 true Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`

## What the Suite Executes

1. `BP_SLZR.ExportAllBlueprints` (skipped when `SkipExport=true`)
2. `BP_SLZR.ValidateConverterReady`
3. `BP_SLZR.AuditAnimationCurves`

## Artifacts Produced

- Export batch directory:
  - `Saved/BlueprintExports/BP_SLZR_All_<timestamp>/`
- Validation report:
  - `Saved/BlueprintExports/BP_SLZR_All_<timestamp>/BP_SLZR_ValidationReport_<timestamp>.json`
- Curve audit report:
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_<timestamp>.json`
- Suite run manifest:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_<timestamp>.json`

## Baseline Gates

The suite compares current metrics against `REGRESSION_BASELINE.json`:

- required validation gates that must be `true`
- minimum validation metrics
- minimum curve-audit metrics

Current baseline also enforces schema-shape gates for networking/replication (`variableReplicationShape`, `functionNetworkShape`), compiler fallback payload (`compilerIRFallbackShape`), and macro dependency closure payload (`macroDependencyClosureShape`).

Suite pass/fail is recorded in `suitePass` and `failures` in the suite manifest.

## Verification Rules

- Build success must be confirmed from UBT output/log.
- Regression success must be confirmed from report artifacts and `Saved/Logs/LyraStarterGame.log`.
- Shell timeout alone is not a failure signal for Unreal commandlets.
- If editor/cmd windows linger after completion, treat that as harness failure and update the launcher or cleanup logic before continuing.

## Update Discipline After Every Run

1. Add/refresh evidence in `CONVERTER_READY_TODO.md`.
2. Append a dated entry to `REGRESSION_LOG.md`.
3. If contract scope changed, update `CONVERTER_READY_EXTRACTION_SPEC.md`.
