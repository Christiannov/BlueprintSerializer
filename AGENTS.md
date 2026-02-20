# BlueprintSerializer Agent Guide

> **START HERE — before reading anything else, read the universal core context file:**
>
> **`Plugins/BlueprintSerializer/blueprintserializer_cortext.md`**
>
> That file is the single source of truth for this plugin: build rules, regression
> workflow, architectural patterns, key source files, and baseline metrics.
> Everything below is a quick-reference supplement to it.

---

## Canonical References (read in this order)

1. **`blueprintserializer_cortext.md`** ← universal context, always first
2. `CONVERTER_READY_TODO.md` — live backlog
3. `REGRESSION_LOG.md` — run history and lessons
4. `REGRESSION_BASELINE.json` — current metric thresholds
5. `REGRESSION_HARNESS.md` — harness internals
6. `FULL_EXTRACTION_DEFINITION.md` — complete extraction spec
7. `CONVERTER_READY_EXTRACTION_SPEC.md` — converter-ready scope

---

## Required Workflow for Any Non-Trivial Change

1. Read `blueprintserializer_cortext.md` (build rule, patterns, baseline)
2. Make code changes
3. Build — **host project editor target, not plugin name** (see cortext for command)
4. Run regression suite: `powershell -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1`
5. Confirm `suitePass=True` in the JSON artifact — not from shell exit code
6. Update `REGRESSION_BASELINE.json` if new metrics were added
7. Append entry to `REGRESSION_LOG.md`
8. Commit plugin + superproject with descriptive message

---

## Anti-Regression Rules

- Do not claim success unless artifact files exist and contain expected metrics
- `suitePass=True` in `BP_SLZR_RegressionRun_<timestamp>.json` is the only valid success signal
- Treat Unreal command timeouts as inconclusive; confirm via log and report JSON files
- Never mark a task `done` without corpus evidence from a passing regression run

---

## Scope Priority

- **Primary:** C++ conversion fidelity — extraction completeness, node meta enrichment, dependency closure
- **Secondary:** Editor-heavy animation authoring — human-led, automation assists only

Keep `CONVERTER_READY_TODO.md` in normalized backlog mode so new sessions inherit priority without rereading the full log.
