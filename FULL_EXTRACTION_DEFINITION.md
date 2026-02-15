# BlueprintSerializer Full Extraction Definition

This document defines what "full extraction" means for BlueprintSerializer output.
It is the canonical contract so we do not need to redefine scope in chat.

For Blueprint->C++ converter fidelity requirements, see:
`CONVERTER_READY_EXTRACTION_SPEC.md`

For live task progress and metrics, see:
`CONVERTER_READY_TODO.md`

## 1) Intent

Full extraction means:
- The JSON schema is complete and stable for the Blueprint type being exported.
- Data captured from editor-visible sources is as complete as available from Unreal APIs.
- Missing source data results in empty values/arrays, not missing required schema keys.

This is a contract document, not a status report.
Current progress and metrics should be tracked in a separate run report.

## 2) Extraction Contract

## 2.1 Required for every Blueprint export

The following top-level fields are required for all exports:
- `schemaVersion`, `blueprintPath`, `blueprintName`, `parentClassName`, `generatedClassName`
- `totalNodeCount`, `extractionTimestamp`
- `implementedInterfaces`, `variables`, `functions`, `components`, `eventNodes`, `graphNodes`
- `detailedVariables`, `detailedFunctions`, `detailedComponents`
- `assetReferences`
- `structuredGraphs`
- `coverage`
- `graphsSummary`

Behavior rules:
- `structuredGraphs`, `coverage`, and `graphsSummary` must always exist (even if empty).
- Collections should be deterministic where practical (sorted maps, de-duped references).
- Extraction must be non-fatal when data is missing or a type is unavailable.

## 2.2 Required for AnimBlueprint exports

For `UAnimBlueprint`, these fields are additionally required:
- `animationVariables`
- `animGraph`
- `animationAssets`
- `controlRigs`
- `gameplayTags`
- `fastPathAnalysis`

Minimum content expectations:
- Variable extraction includes resolved default values when available.
- Anim graph extraction includes root nodes, state machines, transitions, linked layers, and pin/flow data.
- Animation assets include notifies/curves/montage/blendspace data when source assets are discoverable.
- Control rigs are extracted when ControlRig references are discoverable from AnimBP node/property paths.
- Fast-path summary is present and computed from available node metadata.

## 2.3 Required for ControlRig payloads

Each `controlRigs[]` entry must include:
- `rigName`, `skeletonPath`
- `setupEventNodes`, `forwardSolveNodes`, `backwardSolveNodes`
- `rigVMConnections`

Stability rules:
- RigVM node IDs must be stable per export input (deterministic generation is acceptable).
- Missing graphs must produce empty arrays, not missing fields.

## 3) Full Extraction Quality Gates

An export run is considered "full" only if all gates pass:

1. Build gate
- Plugin/module compiles successfully for the target engine version.

2. Export gate
- Batch/target export completes without fatal errors.

3. Schema gate
- Required fields (section 2) are present for each exported Blueprint type.

4. Data gate
- Known reference assets in sanity set resolve expected linked data (for example: known AnimBPs resolve known ControlRigs).

5. Determinism gate
- Output ordering is stable enough for diff-based review.

## 4) Out of Scope (not required for "full")

The following are explicitly not required unless future contract revisions add them:
- Runtime-only state not available in editor/CDO.
- Proprietary plugin internals with no public API access.
- Perfect semantic interpretation of every custom node class.

## 5) Change Control

When behavior changes:
- Backward-compatible additions are preferred.
- Breaking field renames/removals require a schema version bump.
- Update this document only when the contract changes.
- Track run-by-run pass/fail and metrics in a separate status artifact.
