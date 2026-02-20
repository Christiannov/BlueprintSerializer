# BlueprintSerializer Converter-Ready TODO

Live tracker for closing gaps in `CONVERTER_READY_EXTRACTION_SPEC.md`.

Last updated: 2026-02-20
Owner: BlueprintSerializer team

## Status legend

- `done` = implemented and validated against the current validation corpus.
- `in_progress` = partially implemented or implemented without full validation.
- `todo` = not implemented yet.
- `blocked` = cannot proceed due to engine/API/project constraint.

Mandate override (2026-02-18):
- All focus/work items are intentionally normalized to `todo` to keep a single prioritized backlog state.
- "Left off" context in notes preserves prior state and latest evidence.
- Primary mandate is AI-first C++ conversion fidelity work; editor-heavy animation authoring tasks stay secondary.

## Baseline snapshot (latest batch)

Export batch sampled: `Saved/BlueprintExports/*_20260214_1305*.json`

- Files exported: `233`
- Schema stability: `structuredGraphs/coverage/graphsSummary` missing in `0` files
- AnimBPs detected: `20`
- AnimBPs with `animationAssets`: `1/20` (total assets: `1`)
- AnimBPs with `controlRigs`: `3/20` (total rigs: `3`)
- AnimBPs with `gameplayTags`: `1/20` (total tags: `5`)
- Animation variable defaults populated: `116/117`
- Detailed functions with non-empty `returnType`: `0/307`
- Detailed components with resolved parent (non-Unknown): `0/38`
- Detailed components with asset references: `0/38`

## Current smoke validation (2026-02-15)

- Build: `LyraEditor Win64 Development` succeeds with `-NoUBA`.
- Command check: `BP_SLZR.Count` reports `Found 485 Blueprints in project`.
- Command check: `BP_SLZR.Serialize /Game/Weapons/B_Weapon.B_Weapon` succeeds and writes:
  - `Saved/BlueprintExports/BP_SLZR_Blueprint_B_Weapon_20260215_180711.json`
- Command check: `BP_SLZR.GenerateContext /Game/Weapons/B_Weapon.B_Weapon` succeeds and writes:
  - `Saved/BlueprintSerializer/LLMContext/BP_SLZR_Context_B_Weapon_20260215_183348.md`
- Command check: `BP_SLZR.Serialize /Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base.ABP_Mannequin_Base` succeeds and writes:
  - `Saved/BlueprintExports/BP_SLZR_Blueprint_ABP_Mannequin_Base_20260215_185403.json`
- Command check: `BP_SLZR.GenerateContext /Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base.ABP_Mannequin_Base` succeeds and writes:
  - `Saved/BlueprintSerializer/LLMContext/BP_SLZR_Context_ABP_Mannequin_Base_20260215_185811.md`
- Command check: `BP_SLZR.Serialize /Game/Tools/B_Tool_Repeater.B_Tool_Repeater` succeeds and writes:
  - `Saved/BlueprintExports/BP_SLZR_Blueprint_B_Tool_Repeater_20260215_190217.json`
- Spot-check evidence from the B_Weapon export confirms new converter fields are present:
  - `parentClassPath`, `generatedClassPath`, `implementedInterfacePaths`, `importedNamespaces`
  - `delegateSignatures`, `timelines`, `unsupportedNodeTypes`, `partiallySupportedNodeTypes`
  - enriched `detailedVariables` / `detailedFunctions` / `detailedComponents` metadata

## Extended smoke validation batch (2026-02-16)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_Blueprint_*_20260216_11*.json` (`5` files)
- `Saved/BlueprintSerializer/LLMContext/BP_SLZR_Context_*_20260216_11*.md` (`3` files)

- Command re-check: `BP_SLZR.Count` reports `Found 485 Blueprints in project`.
- Additional serialize checks succeed and write:
  - `Saved/BlueprintExports/BP_SLZR_Blueprint_B_LyraGameMode_20260216_110049.json`
  - `Saved/BlueprintExports/BP_SLZR_Blueprint_B_Hero_Default_20260216_110358.json`
  - `Saved/BlueprintExports/BP_SLZR_Blueprint_B_WeaponFire_20260216_110735.json`
  - `Saved/BlueprintExports/BP_SLZR_Blueprint_B_MusicManagerComponent_Base_20260216_111114.json`
  - `Saved/BlueprintExports/BP_SLZR_Blueprint_ABP_Weap_Shotgun_20260216_111451.json`
- Additional context checks succeed and write:
  - `Saved/BlueprintSerializer/LLMContext/BP_SLZR_Context_B_Hero_Default_20260216_111831.md`
  - `Saved/BlueprintSerializer/LLMContext/BP_SLZR_Context_ABP_Weap_Shotgun_20260216_112210.md`
  - `Saved/BlueprintSerializer/LLMContext/BP_SLZR_Context_B_LyraGameMode_20260216_112546.md`
- Converter key stability in this batch: required converter-facing keys missing in `0/5` exports.
  - Checked keys: `parentClassPath`, `generatedClassPath`, `implementedInterfacePaths`, `importedNamespaces`, `delegateSignatures`, `timelines`, `unsupportedNodeTypes`, `partiallySupportedNodeTypes`, `detailedVariables`, `detailedFunctions`, `detailedComponents`.
- Node support matrix signal in this batch:
  - `unsupportedNodeTypes` appears in `4/5` exports (total entries: `6`).
  - `partiallySupportedNodeTypes` appears in `3/5` exports (total entries: `16`).

## Full-project export validation (2026-02-16)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260216_4/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260216_4/BP_SLZR_Manifest_20260216_131825.json`

- Command check: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260216_4` completes with `Success: 485, Failed: 0`.
- Manifest/output parity:
  - Manifest totals: `total=485`, `successCount=485`, `failCount=0`.
  - Output files: `485` blueprint JSON files + `1` manifest file.
- Filename collision fix evidence:
  - Duplicate asset name `Paint_Pawn` now emits two distinct files via path-hash suffix:
    - `BP_SLZR_Blueprint_Paint_Pawn_b06cb1e7_20260216_130153.json`
    - `BP_SLZR_Blueprint_Paint_Pawn_91d07f62_20260216_130154.json`
- Encoding consistency fix evidence:
  - UTF-8 parseability across full batch: `0` parse failures (`485/485` parseable as UTF-8 JSON).
- Converter key stability in full batch:
  - Required converter-facing keys missing in `0/485` exports.
  - Checked keys: `parentClassPath`, `generatedClassPath`, `implementedInterfacePaths`, `importedNamespaces`, `delegateSignatures`, `timelines`, `unsupportedNodeTypes`, `partiallySupportedNodeTypes`, `detailedVariables`, `detailedFunctions`, `detailedComponents`, `structuredGraphs`, `coverage`, `graphsSummary`.
- Additional full-batch metrics:
  - `unsupportedNodeTypes` appears in `164/485` exports (total entries: `200`).
  - `partiallySupportedNodeTypes` appears in `241/485` exports (total entries: `1027`).
  - `detailedFunctions` with non-empty `returnType`: `39/936`.
  - `detailedComponents` with resolved parent: `314/613`.
  - `detailedComponents` with asset references: `311/613`.
  - AnimBP sample in this full batch: `25`; with `animationAssets`: `1/25` (total assets `1`); with `controlRigs`: `3/25` (total rigs `3`).

## Semantic parity expansion batch (2026-02-16)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260216_5/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260216_5/BP_SLZR_Manifest_20260216_151457.json`
- Spot-check single export: `Saved/BlueprintExports/BP_SLZR_Blueprint_B_Weapon_d2da07c8_20260216_150932.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260216_5` -> `Success: 485, Failed: 0`.
- Full-batch parseability: `485/485` export files parse as UTF-8 JSON.
- New parity fields now emitted and stable in full batch:
  - `classSpecifiers`, `classConfigName`, `classDefaultValues`, `classDefaultValueDelta`, `dependencyClosure`
  - required-key missing count for this set: `0/485`
- Declaration/specifier coverage in this batch:
  - `detailedVariables` with non-empty `declarationSpecifiers`: `3133/3133`
  - `detailedFunctions` with non-empty `declarationSpecifiers`: `936/936`
  - `detailedFunctions` with explicit `returnType` populated (`void` included): `936/936`
- Class default / dependency closure coverage:
  - exports with non-empty `classDefaultValues`: `461/485`
  - exports with non-empty `classDefaultValueDelta`: `419/485`
  - exports with non-empty `dependencyClosure`: `485/485`

## User type schema expansion batch (2026-02-16)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260216_6/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260216_6/BP_SLZR_Manifest_20260216_154433.json`
- Context smoke output: `Saved/BlueprintSerializer/LLMContext/BP_SLZR_Context_B_Weapon_d2da07c8_20260216_155154.md`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260216_6` -> `Success: 485, Failed: 0`.
- Full-batch parseability: `485/485` export files parse as UTF-8 JSON.
- New user-type/schema fields now emitted and stable in full batch:
  - `userDefinedStructSchemas`, `userDefinedEnumSchemas`
  - required-key missing count for this set (including prior parity fields): `0/485`
- User-defined type schema coverage in this batch:
  - exports with non-empty `userDefinedStructSchemas`: `42/485` (total extracted fields: `575`)
  - exports with non-empty `userDefinedEnumSchemas`: `53/485` (total extracted enumerators: `398`)
- Dependency closure refinement:
  - `dependencyClosure.includeHints` emitted and non-empty in `485/485` exports.
  - `dependencyClosure` remains non-empty in `485/485` exports.

## Inheritable component override expansion batch (2026-02-17)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260216_9/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260216_9/BP_SLZR_Manifest_20260216_202941.json`
- Override sample: `Saved/BlueprintExports/BP_SLZR_All_20260216_9/BP_SLZR_Blueprint_BakableTwistyBoxDemo_5d3277da_20260216_202920.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260216_9` -> `Success: 485, Failed: 0`.
- Full-batch parseability: `485/485` export files parse as UTF-8 JSON.
- Existing parity keys remain stable (including prior class/default/dependency/user-type fields): `0/485` missing required-key checks.
- Component hierarchy/override coverage in this batch:
  - total `detailedComponents`: `818`
  - `isInherited=true`: `205/818`
  - `hasInheritableOverride=true`: `18/818` (appears in `12/485` exports)
  - non-empty `inheritableOverrideProperties`: `16/818`
- Override delta payload now emitted per component (when present):
  - `hasInheritableOverride`, `inheritableOwnerClassPath`, `inheritableComponentGuid`
  - `inheritableSourceTemplatePath`, `inheritableOverrideTemplatePath`
  - `inheritableOverrideProperties`, `inheritableOverrideValues`, `inheritableParentValues`

## Class config + validation gate automation batch (2026-02-17)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260217_1/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260217_1/BP_SLZR_Manifest_20260216_221309.json`
- Validation report: `Saved/BlueprintExports/BP_SLZR_All_20260217_1/BP_SLZR_ValidationReport_20260217_005135.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260217_1` -> `Success: 485, Failed: 0`.
- Added class config parity map to every export:
  - `classConfigFlags` object with keys:
    - `isConfigClass`, `isDefaultConfig`, `isPerObjectConfig`, `isGlobalUserConfig`, `isProjectUserConfig`, `isPerPlatformConfig`, `configDoesNotCheckDefaults`, `hasConfigName`
  - coverage: `485/485` exports include `classConfigFlags`; missing required class-config keys: `0/485`
- Automated converter validation command implemented and validated:
  - command: `BP_SLZR.ValidateConverterReady [ExportDir] [ReportPath]`
  - gate result for this batch: `overallPass=true`
  - gate checks now automated for: manifest parity, parse errors, required key presence, declaration-specifier coverage, class-config shape, component-override shape, dependency closure/include-hint coverage.
- Re-validated on later full export batch with latest command logic:
  - `BP_SLZR.ValidateConverterReady` on `Saved/BlueprintExports/BP_SLZR_All_20260217_2`
  - report: `Saved/BlueprintExports/BP_SLZR_All_20260217_2/BP_SLZR_ValidationReport_20260217_024410.json`
  - result remained `overallPass=true` with full gate pass.

## Dependency include quality batch (2026-02-17)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260217_3/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260217_3/BP_SLZR_Manifest_20260217_104541.json`
- Validation report: `Saved/BlueprintExports/BP_SLZR_All_20260217_3/BP_SLZR_ValidationReport_20260217_122155.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260217_3` -> `Success: 485, Failed: 0`.
- Dependency closure include quality improvements:
  - canonicalized `/Script/*Default__*` script-object paths to class-symbol paths in closure (`Default__` class noise removed)
  - added `dependencyClosure.nativeIncludeHints` from reflected `ModuleRelativePath` metadata
  - `dependencyClosure.includeHints` now prefers native metadata-derived headers and falls back only when native header resolution is unavailable
- Validation metrics on this batch:
  - `overallPass=true`
  - `includeHintsNonEmpty=485/485`
  - `nativeIncludeHintsNonEmpty=485/485`
  - `exportsWithDefaultObjectClassPaths=0/485`
  - `exportsWithDefaultObjectIncludeHints=0/485`

## ControlRig hierarchy + validation dedupe batch (2026-02-17)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260217_4/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260217_4/BP_SLZR_Manifest_20260217_133208.json`
- Validation report: `Saved/BlueprintExports/BP_SLZR_All_20260217_4/BP_SLZR_ValidationReport_20260217_145401.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260217_4` -> `Success: 485, Failed: 0`.
- ControlRig payload depth increased from hierarchy data:
  - `controls`, `bones`, `enabledFeatures`, `featureSettings`, `rigProperties` now populated when ControlRig hierarchy data is available
  - sample: `BP_SLZR_Blueprint_ABP_Mannequin_Base_da729c0c_20260217_133208.json` includes non-empty `controls` and `bones`
- Validation command robustness improved for reused export directories:
  - `BP_SLZR.ValidateConverterReady` now de-duplicates repeated export JSONs by `blueprintPath` and latest `extractionTimestamp`
  - report now includes `rawBlueprintFileCount` and `duplicateBlueprintExportsIgnored`
- Validation metrics on this batch:
  - `overallPass=true`
  - `blueprintFileCount=485` with `rawBlueprintFileCount=970` and `duplicateBlueprintExportsIgnored=485`
  - `exportsWithControlRigs=3`, `controlRigsWithControls=1`, `controlRigsWithBones=3`, `controlRigsWithControlToBoneMap=0`

## ControlRig control-bone mapping follow-up batch (2026-02-17)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260217_5/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260217_5/BP_SLZR_Manifest_20260217_151848.json`
- Validation report: `Saved/BlueprintExports/BP_SLZR_All_20260217_5/BP_SLZR_ValidationReport_20260217_161849.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260217_5` -> `Success: 485, Failed: 0`.
- ControlRig map follow-up:
  - added hierarchy/name-heuristic control-bone mapping fallback (`Left/Right*KneePV*` -> `calf_l/calf_r`, `PelvisCtrl` -> `pelvis`, `RootCtrl` -> `root` in sample)
  - sample evidence: `BP_SLZR_Blueprint_ABP_Mannequin_Base_da729c0c_20260217_151847.json` now emits non-empty `controlToBoneMap` with `mappedControlCount=4`
- Validation metrics on this batch:
  - `overallPass=true`
  - `rawBlueprintFileCount=485`, `duplicateBlueprintExportsIgnored=0`
  - `exportsWithControlRigs=3`, `controlRigsWithControls=1`, `controlRigsWithBones=3`, `controlRigsWithControlToBoneMap=1`

## Transition + notify payload enrichment batch (2026-02-17)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260217_6/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260217_6/BP_SLZR_Manifest_20260217_183118.json`
- Validation report: `Saved/BlueprintExports/BP_SLZR_All_20260217_6/BP_SLZR_ValidationReport_20260217_195321.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260217_6` -> `Success: 485, Failed: 0`.
- Transition payload enrichment:
  - explicitly populated transition fields from editor node data: `blendMode`, `priority`, `blendOutTriggerTime`, `notifyStateIndex`
  - sample evidence: `BP_SLZR_Blueprint_ABP_Mannequin_Base_da729c0c_20260217_183118.json` now shows populated transition fields (`blendMode=HermiteCubic`, `priority=1`, etc.)
- Notify payload enrichment:
  - `ExtractAnimSequenceNotifies` now fills `trackIndex`, `trackName`, `triggeredEventName`, and derives `eventParameters` from extracted notify property keys
  - current full-batch remains limited by upstream anim-asset breadth (`animationNotifiesTotal=0`), so these fields are implemented but not yet broadly exercised in this corpus
- Validation metrics on this batch:
  - `overallPass=true`
  - transition coverage metrics: `animationTransitionsTotal=45`, `animationTransitionsWithBlendMode=45`, `animationTransitionsWithPriority=45`, `animationTransitionsWithBlendOutTriggerTime=45`, `animationTransitionsWithNotifyStateIndex=45`
  - notify metrics: `animationAssetsTotal=2`, `animationNotifiesTotal=0`

## Anim dependency harvest follow-up batch (2026-02-18)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260218_1/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260218_1/BP_SLZR_Manifest_20260217_202028.json`
- Validation report: `Saved/BlueprintExports/BP_SLZR_All_20260218_1/BP_SLZR_ValidationReport_20260217_211948.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260218_1` -> `Success: 485, Failed: 0`.
- Anim asset breadth uplift from AssetRegistry dependency harvest in `ExtractAnimBlueprintData`:
  - now seeds AnimBP candidate set with likely anim/control-rig dependency packages (hard+soft package refs)
  - lifts full-batch extraction from sparse anim assets/notifies to broad coverage
- Validation metrics on this batch:
  - `overallPass=true`
  - `animationAssetsTotal=409` (up from `2` in `_20260217_6`)
  - `animationNotifiesTotal=2960`, with `trackName=2960`, `triggeredEventName=2960`, `eventParameters=2960`
  - transitions remain fully populated: `45/45` for blend/priority/blendOut/notifyState fields
- AnimBP corpus comparison (regex audit over exported files):
  - `_20260217_6`: `AnimBPs=23`, with non-zero `totalAnimAssets=2`, `totalControlRigs=3`
  - `_20260218_1`: `AnimBPs=23`, with non-zero `totalAnimAssets=11`, `totalControlRigs=3`
- Follow-up coverage report from `_20260218_5` exports:
  - artifact: `Saved/BlueprintExports/BP_SLZR_AnimBPAssetCoverage_20260218_1.json`
  - `23` AnimBlueprint-like exports total; `11` with non-zero `animationAssets`, `12` with zero.
  - Highest-yield AnimBPs are locomotion layer assets (`66-69` assets each for core rifle/pistol/unarmed layer blueprints).
  - Remaining zero-asset set is dominated by wrapper/reference-style AnimBPs (for example ref-pose/retarget/tutorial/interface/post-process patterns).

## Curve payload expansion batch (2026-02-18)

Batch sampled:

- `Saved/BlueprintExports/BP_SLZR_All_20260218_5/*.json`
- Manifest: `Saved/BlueprintExports/BP_SLZR_All_20260218_5/BP_SLZR_Manifest_20260218_044451.json`
- Validation report: `Saved/BlueprintExports/BP_SLZR_All_20260218_5/BP_SLZR_ValidationReport_20260218_044457.json`
- Spot-check sample: `Saved/BlueprintExports/BP_SLZR_All_20260218_5/BP_SLZR_Blueprint_ABP_UnarmedAnimLayers_Feminine_a00a73b3_20260218_044450.json`

- Full export command succeeds: `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260218_5` -> `Success: 485, Failed: 0`.
- Validation command succeeds: `BP_SLZR.ValidateConverterReady` on this batch -> `overallPass=true`.
- Curve payload expansion now emitted in `animationCurves` shape:
  - `curveAssetPath`, `interpolationMode`, and `axisType` now populated in the observed float-curve corpus (`315/315` each).
  - `vectorValues`, `transformValues`, `affectedMaterials`, and `affectedMorphTargets` now serialized as explicit fields (present in schema even when empty).
- Curve metrics in this batch:
  - `animationCurvesTotal=315`
  - `animationCurvesWithCurveAssetPath=315`
  - `animationCurvesWithInterpolationMode=315`
  - `animationCurvesWithAxisType=315`
  - `animationCurvesWithVectorValues=0`
  - `animationCurvesWithTransformValues=0`
  - `animationCurvesWithAffectedMaterials=0`
  - `animationCurvesWithAffectedMorphTargets=0`
  - `animationCurvesFloatType=315`
  - `animationCurvesVectorType=0`
  - `animationCurvesTransformType=0`
- Interpretation:
  - CR-013 schema payload expansion is implemented and validated for float-curve fields.
  - No vector/transform curve instances were observed in this corpus; non-float curve population remains a corpus/coverage follow-up.

## Project-wide animation curve corpus audit (2026-02-18)

Audit artifact:

- `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_1.json`

- Command check: `BP_SLZR.AuditAnimationCurves Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_1.json` succeeds and writes audit report.
- Project-wide AnimSequence curve corpus metrics from the audit:
  - `animSequenceAssetsTotal=584`
  - `loadedAnimSequences=584`
  - `sequencesWithAnyCurves=233`
  - `sequencesWithFloatCurves=233`
  - `sequencesWithTransformCurves=0`
  - `sequencesWithMaterialCurves=0`
  - `sequencesWithMorphCurves=0`
  - `floatCurvesTotal=281`
  - `transformCurvesTotal=0`
  - `materialCurvesTotal=0`
  - `morphCurvesTotal=0`
- Audit candidate arrays are empty:
  - `transformCurveAssets=[]`
  - `materialCurveAssets=[]`
  - `morphCurveAssets=[]`
  - `candidateSequencePaths=[]`
- Interpretation:
  - Current project corpus does not contain non-float AnimSequence curve instances to exercise CR-013 vector/transform/material/morph value population paths.
  - CR-013 implementation work is present, but full non-float runtime population validation is blocked on corpus availability.

## C++ complete conversion focus (primary)

This section defines the must-have subset for "things usually done in C++".
We should treat these as blocking items before calling the serializer C++-complete.

| Focus ID | Goal | Mapped work items | Status |
|---|---|---|---|
| CXX-001 | Deterministic class/function graph IR for codegen | CR-001, CR-016, CR-018, CR-026 | todo |
| CXX-002 | Lossless type/signature model (params/returns/containers/object paths) | CR-007, CR-022, CR-025 | todo |
| CXX-003 | Runtime semantics parity (RPC/replication/authority/delegates/latent/timeline) | CR-008, CR-019, CR-020 | todo |
| CXX-004 | Class defaults + component hierarchy/defaults/overrides parity | CR-009, CR-010, CR-023, CR-024 | todo |
| CXX-005 | Symbol/dependency closure for generated C++ includes/modules | CR-014, CR-025, CR-028 | todo |
| CXX-006 | Function implementation parity for real project patterns | CR-021, CR-027 | todo |
| CXX-007 | UPROPERTY/UFUNCTION/class specifier parity for generated C++ declarations | CR-023, CR-029 | todo |
| CXX-008 | Unsupported/custom node fallback path for correctness | CR-035, CR-037 | todo |

Notes:
- Animation depth items remain important for AnimBP conversion, but they are secondary to the CXX-* goals for gameplay/class conversion.
- CXX focus rows were previously `in_progress` before mandate reset; reset keeps priority ordering explicit while preserving left-off context in mapped CR notes.

## Work items

| ID | Area | Task | Status | Notes |
|---|---|---|---|---|
| CR-001 | Schema | Keep required stable keys always emitted for all exports | done | `structuredGraphs`, `coverage`, `graphsSummary` stable across all 485 exports |
| CR-002 | Anim vars | Resolve AnimBP variable defaults robustly | todo | left off (in_progress): `116/117` populated; finish edge cases |
| CR-003 | Anim assets | Increase animation asset discovery coverage from graph/rule/layer/property paths | todo | left off (in_progress): dependency-harvest pass lifted coverage to `11/23` AnimBPs with non-zero `totalAnimAssets` (from `2/23` in prior full batch); continue improving long-tail detection |
| CR-004 | ControlRig linkage | Discover and extract referenced ControlRigs from AnimBP nodes/properties | todo | left off (in_progress): current latest full-batch corpus: `3/23` AnimBPs with non-zero `totalControlRigs`; maintain while expanding discovery paths |
| CR-005 | ControlRig detail model | Populate `controls`, `bones`, `controlToBoneMap`, feature fields | todo | left off (in_progress): hierarchy-driven `controls`/`bones`/feature maps populated; heuristic control-bone mapping now emits non-empty maps for a subset of rigs, but broader semantic mapping remains pending |
| CR-006 | Gameplay tags | Replace name/type heuristics with richer tag flow extraction | todo | left off (in_progress): graph/pin/property gameplay-tag flow extraction with `tagMetadata` context is implemented; validator reads `tagMetadata` (with legacy `metadata` fallback); fresh full regression on 2026-02-19 confirmed `gameplayTagsWithSourceGraphMetadata=24`, `gameplayTagsWithSourceNodeTypeMetadata=24` out of `gameplayTagsTotal=29` — 5 `AnimVariablePrefix` tags carry only `sourceVariable`/`variableType`, absence of `sourceGraph` is correct for that category; ShaderCompileWorker blocker resolved via `-nullrhi` in `Run-RegressionSuite.ps1` |
| CR-007 | Function signatures | Populate explicit return type + richer typed parameter metadata | todo | left off (in_progress): return/object-path metadata now exported; needs wider corpus validation |
| CR-008 | Networking semantics | Export RPC kind/reliability/validation + RepNotify linkage | todo | left off (in_progress): network/replication fields exported and validator now enforces shape gates (`variableReplicationShape`, `functionNetworkShape`); latest batch reports zero non-default RPC usage in current corpus |
| CR-009 | Components | Extract real component hierarchy and actual default values | todo | left off (in_progress): parent/root/inherited metadata + default property extraction implemented |
| CR-010 | Component deps | Populate component-level asset/class references | todo | left off (in_progress): component asset references now emitted; validate at scale |
| CR-011 | Transition fidelity | Populate full transition fields (blend mode, priority, notify index, etc.) | todo | left off (in_progress): explicit transition fields now populated (`blendMode`, `priority`, `blendOutTriggerTime`, `notifyStateIndex`) and validated at `45/45` in latest batch; deeper event/edge-case semantics remain |
| CR-012 | Notify fidelity | Populate track/event payload (`trackName`, `eventParameters`, etc.) | todo | left off (in_progress): notify extraction now populates `trackIndex`/`trackName`/`triggeredEventName` and derived `eventParameters`; broader runtime evidence awaits higher anim-asset/notify discovery coverage |
| CR-013 | Curve fidelity | Add vector/transform/material/morph curve extraction where available | todo | left off (blocked): curve payload fields expanded and emitted (`curveAssetPath`, `interpolationMode`, `axisType`, `vectorValues`, `transformValues`, `affectedMaterials`, `affectedMorphTargets`); full-batch and project-wide curve audit still show float-only corpus (`sequencesWithTransformCurves=0`, `sequencesWithMaterialCurves=0`, `sequencesWithMorphCurves=0`) |
| CR-014 | Dependency closure | Emit converter-facing dependency closure manifest (types/modules/includes) | todo | left off (in_progress): emits class/struct/enum/interface/asset/control-rig/module sets plus `includeHints` and `nativeIncludeHints`; include/module fidelity still needs deeper compile-loop tuning |
| CR-015 | Tooling parity | Implement missing module entrypoints (`BP_SLZR.Serialize`, selected serialize, context generation) | done | `BP_SLZR.Serialize`/`GenerateContext`/`Count`/`ExportAllBlueprints` all wired and validated |
| CR-016 | Validation gates | Automate converter-ready gate checks and publish pass/fail report per run | done | `BP_SLZR.ValidateConverterReady` + `Run-RegressionSuite.ps1` harness; 14 gates, `suitePass=True` enforced |
| CR-017 | Legacy path cleanup | Replace/remove `AnalyzeNodeDetails` temporary stub path to avoid partial legacy behavior | done | `AnalyzeNodeDetails` stub removed; no legacy call sites remain |
| CR-018 | Command surface consistency | Unify command entrypoints so all public commands use the same extraction/export pipeline | todo | left off (in_progress): major path now unified; legacy extractor command surface still exists |
| CR-019 | Delegate model | Extract dispatcher declarations + delegate signature graphs as converter IR | todo | left off (in_progress): delegate signatures + delegate graph coverage now exported |
| CR-020 | Timeline model | Extract `UTimelineTemplate` tracks/keys/events/length/loop settings (not only node name) | todo | left off (in_progress): timeline template/track/key metadata now exported |
| CR-021 | Function locals | Extract function-local variable declarations/defaults/scopes from function entry metadata | todo | left off (in_progress): local variable declarations now exported |
| CR-022 | Type fidelity | Export full variable type model (container kind, key/value type, object/class path) for member vars/functions | todo | left off (in_progress): added category/subcategory/object-path/container flags |
| CR-023 | Class/component overrides | Export inheritable component overrides and class-level config flags relevant to generated C++ | todo | left off (in_progress): inheritable override metadata/delta fields plus class-level `classConfigFlags` now emitted; expand override-depth edge cases |
| CR-024 | Class defaults | Export class default object (CDO) property delta for converter-visible gameplay settings | todo | left off (in_progress): `classDefaultValues` + `classDefaultValueDelta` now emitted; filtering/scope tuning still pending |
| CR-025 | Symbol identity | Export fully-qualified paths for parent classes, interfaces, types, and callable owners | todo | left off (in_progress): class/interface/type paths now exported in primary schema |
| CR-026 | Graph coverage | Include delegate/collapsed/other relevant editable graphs beyond Ubergraph/Function/Macro | todo | left off (in_progress): added delegate/event coverage and construction role tagging |
| CR-027 | Override semantics | Export explicit override/parent-call semantics for events and overridden functions | todo | left off (in_progress): `bIsOverride`/`bCallsParent` + parent-call node metadata exported |
| CR-028 | User type schemas | Export user-defined struct/enum schema details required for C++ type generation/mapping | todo | left off (in_progress): `userDefinedStructSchemas` / `userDefinedEnumSchemas` now emitted; richer defaults/metadata still pending |
| CR-029 | Declaration specifiers | Export class/property/function specifier-level metadata needed for header generation parity | todo | left off (in_progress): class-level `classSpecifiers` plus variable/function `declarationSpecifiers` now emitted; deeper UHT parity still pending |

## Minor C++ parity gaps (polish)

These are smaller but still useful for high-confidence, low-friction conversion output.

| ID | Area | Task | Status | Notes |
|---|---|---|---|---|
| CR-030 | Blueprint metadata | Export `BlueprintNamespace`, imported namespaces, and additional class-level metadata used for symbol resolution | done | `blueprintNamespace`/`importedNamespaces` emitted and stable across full corpus |
| CR-031 | Construction role tagging | Explicitly tag/identify construction-script graph role in structured graph output | done | Construction-script graphs tagged as `ConstructionScript` in structured graph output |
| CR-032 | Determinism | Sort top-level analyzed Blueprint list and other non-semantic arrays for stable cross-run diffs | done | Asset list and export arrays sorted for stable cross-run diffs |
| CR-033 | Schema consistency | Normalize key casing/schema shapes where currently mixed (for example coverage object variants) | done | Removed all snake_case alias keys (`total_nodes`, `by_type`, `unknown_types`, `unknownNodeTypes`) from all 4 `coverage` emission sites; normalized node-type map key to `nodeTypeCounts` everywhere; validated `suitePass=True` (run 20260220_104354) |
| CR-034 | Model consolidation | Consolidate legacy vs active data model surfaces to prevent schema drift (e.g. duplicate type definitions) | todo | `FBS_GraphData` (legacy string format) vs `FBS_GraphData_Ext` (modern typed) coexist; `ExtractStructuredGraphsExt()` overrides `structuredGraphs` output; no duplicate USTRUCTs; low urgency — `FBS_GraphData` removal is a breaking internal refactor with no converter benefit right now |
| CR-035 | Compiler IR fallback | Export optional compiled-script/bytecode metadata for nodes that cannot be deterministically lowered from graph data alone | in_progress | `compilerIRFallback` shape enforced (`exportsWithCompilerIRFallbackShape=485`, `exportsWithBytecodeFallback=205`, `compilerIRFallbackBytecodeFunctionCountTotal=841`); `exportsWithUnsupportedNodeFallback` now **0** (was 164) after Task 51 eliminated EdGraphNode_Comment and AnimGraphNode_* false positives; deeper node-to-bytecode mapping remains pending |
| CR-036 | Macro dependency fidelity | Explicitly resolve/link external macro instance dependencies for robust cross-blueprint conversion | todo | left off (in_progress): dependency closure now emits explicit `macroGraphPaths`/`macroBlueprintPaths` and validator enforces `macroDependencyClosureShape`; latest full run shows populated macro closure metrics (`exportsWithMacroGraphPaths=164`, `macroGraphPathsTotal=510`, `macroBlueprintPathsTotal=266`) |
| CR-037 | Node support matrix | Emit per-export unsupported/partially-supported node report for converter gating and fail-fast behavior | done | `unsupportedNodeTypes`/`partiallySupportedNodeTypes` emitted; all 11 remaining partially-supported K2Node types promoted to fully supported (Task 51); false-positive unsupported count zeroed (`exportsWithUnsupportedNodeFallback=0`) |

## Update protocol

For each completed task:

1. Change status in the table.
2. Add one changelog entry with date and evidence (file path, export sample, or metric delta).
3. If contract scope changed, update `CONVERTER_READY_EXTRACTION_SPEC.md`.

## Changelog

- 2026-02-14: Created tracker and seeded baseline metrics from `_20260214_1305` export batch.
- 2026-02-15: Implemented converter-ready analyzer/model expansions (class paths, interface paths, delegate signatures, timeline data, function net/local/bytecode metadata, component hierarchy defaults, node support matrix) and deterministic export ordering.
- 2026-02-15: Wired `BP_SLZR.Serialize`, selected serialize menu action, and `BP_SLZR.GenerateContext`; implemented markdown LLM context generation with embedded JSON and validated via B_Weapon smoke export/context outputs.
- 2026-02-15: Fixed UE 5.6 compile blockers and deprecations (`SoftObjectProperty` include usage, latent detection without `FUNC_Latent`, `ANY_PACKAGE` call sites, deprecated short-name interface implementation API).
- 2026-02-15: Expanded smoke validation across AnimBP and tool-heavy Blueprint assets (`ABP_Mannequin_Base`, `B_Tool_Repeater`) and confirmed serialize/context commands produce expected output artifacts.
- 2026-02-16: Ran extended cross-domain smoke validation batch (`B_LyraGameMode`, `B_Hero_Default`, `B_WeaponFire`, `B_MusicManagerComponent_Base`, `ABP_Weap_Shotgun`) and confirmed converter-key stability (`0/5` missing checked keys) with updated unsupported/partially-supported node matrix evidence.
- 2026-02-16: Fixed export artifact naming collisions by adding path-hash suffixes to Blueprint/context/audit/roundtrip filenames, preventing overwrites for duplicate Blueprint names from different package paths.
- 2026-02-16: Standardized export/context file encoding to UTF-8 and validated full-batch parseability (`485/485` JSON files parseable as UTF-8) after prior mixed-encoding observation.
- 2026-02-16: Re-ran full-project export validation (`BP_SLZR.ExportAllBlueprints`) with parity checks (`successCount=485`, `output files=485`) and updated full-batch converter metrics.
- 2026-02-16: Added class/property/function declaration-specifier export (`classSpecifiers`, per-variable and per-function `declarationSpecifiers`) and explicit function `void` return typing for deterministic signature reconstruction.
- 2026-02-16: Added class default object layers (`classDefaultValues`, `classDefaultValueDelta`) with parent-safe diff extraction and validated on full export batch.
- 2026-02-16: Added converter-facing `dependencyClosure` object (class/struct/enum/interface/asset/control-rig/module sets) and validated non-empty closure coverage across full export batch (`485/485`).
- 2026-02-16: Expanded dependency closure output with generated `includeHints` and validated full-batch include-hint coverage (`485/485` non-empty).
- 2026-02-16: Added user-defined type schema export (`userDefinedStructSchemas`, `userDefinedEnumSchemas`) and validated coverage on full export batch (`42/485` struct-schema exports, `53/485` enum-schema exports).
- 2026-02-17: Added inheritable component override payload fields (owner/class key/template paths plus per-property parent/override deltas), expanded component extraction across Blueprint class hierarchy, and validated full-batch override coverage (`isInherited=205/818`, `hasInheritableOverride=18/818`, `override deltas=16/818`).
- 2026-02-17: Added class-level config parity map (`classConfigFlags`) and validated full-batch key-shape coverage (`485/485` exports, `0` missing required config-flag keys).
- 2026-02-17: Implemented `BP_SLZR.ValidateConverterReady` gate command to automate manifest/schema/coverage checks and emit a per-run validation report (`overallPass=true` on `BP_SLZR_All_20260217_1`).
- 2026-02-17: Re-ran `BP_SLZR.ValidateConverterReady` on later full batch `BP_SLZR_All_20260217_2` and confirmed sustained gate pass (`overallPass=true`; report `BP_SLZR_ValidationReport_20260217_024410.json`).
- 2026-02-17: Improved dependency closure include quality by canonicalizing `Default__` script-object class paths and emitting metadata-backed `nativeIncludeHints`; validated on `BP_SLZR_All_20260217_3` (`nativeIncludeHintsNonEmpty=485/485`, `exportsWithDefaultObjectClassPaths=0`, `exportsWithDefaultObjectIncludeHints=0`, `overallPass=true`, report `BP_SLZR_ValidationReport_20260217_122155.json`).
- 2026-02-17: Populated ControlRig hierarchy-derived fields (`controls`, `bones`, feature maps, rig properties) and validated improved ControlRig payload coverage on `BP_SLZR_All_20260217_4` (`exportsWithControlRigs=3`, `controlRigsWithControls=1`, `controlRigsWithBones=3`).
- 2026-02-17: Hardened `BP_SLZR.ValidateConverterReady` to de-duplicate repeated exports in reused batch directories via `blueprintPath` + latest `extractionTimestamp`; validated manifest-gate recovery on duplicated directory (`raw=970`, deduped `485`, `overallPass=true`; report `BP_SLZR_ValidationReport_20260217_145401.json`).
- 2026-02-17: Added control-to-bone heuristic fallback mapping in ControlRig extraction and validated non-empty map coverage uplift on fresh batch `BP_SLZR_All_20260217_5` (`controlRigsWithControlToBoneMap=1/3`, sample `mappedControlCount=4` on `ABP_Mannequin_Base`).
- 2026-02-17: Enriched transition extraction with explicit field population (`blendMode`, `priority`, `blendOutTriggerTime`, `notifyStateIndex`) and validated full observed transition coverage in `BP_SLZR_All_20260217_6` (`45/45` for each transition metric in report `BP_SLZR_ValidationReport_20260217_195321.json`).
- 2026-02-17: Enriched notify extraction payload (`trackIndex`, `trackName`, `triggeredEventName`, derived `eventParameters`) and extended validator metrics to report notify/transition population; latest full batch still shows `animationNotifiesTotal=0` due anim-asset breadth limits.
- 2026-02-18: Added AnimBP AssetRegistry dependency-harvest candidate seeding and re-ran full export/validation on `BP_SLZR_All_20260218_1`, raising animation breadth metrics to `animationAssetsTotal=409` and `animationNotifiesTotal=2960` with populated notify payload fields (`trackName`/`triggeredEventName`/`eventParameters` all `2960`).
- 2026-02-18: Updated converter validation reporting with explicit animation transition/notify metric lines and verified transition field population remains fully covered (`45/45` for blend mode, priority, blend-out trigger, notify-state index) in the latest full batch report.
- 2026-02-18: Expanded animation curve payload export and validator metrics (curve asset/interpolation/axis fields plus vector/transform/material/morph slots), then validated on `BP_SLZR_All_20260218_5` with `overallPass=true` and full observed float-curve coverage (`315/315` for curve asset/interpolation/axis fields; vector/transform/material/morph populations remain `0` in this corpus).
- 2026-02-18: Added `BP_SLZR.AuditAnimationCurves` command and ran project-wide AnimSequence curve audit (`BP_SLZR_AnimCurveAudit_20260218_1.json`), confirming the corpus currently contains only float-curve instances (`sequencesWithTransformCurves=0`, `sequencesWithMaterialCurves=0`, `sequencesWithMorphCurves=0`) and no non-float candidate sequence paths.
- 2026-02-18: Added AnimBP asset-coverage audit artifact for latest full export (`BP_SLZR_AnimBPAssetCoverage_20260218_1.json`) to identify remaining zero-asset AnimBPs (`12/23`) versus high-yield locomotion-layer AnimBPs (`66-69` assets each).
- 2026-02-18: Completed CR-017 legacy cleanup by removing the disabled `AnalyzeNodeDetails` stub path from `BlueprintAnalyzer` (declaration + implementation), reducing dead legacy surface and preventing accidental fallback drift.
- 2026-02-18: Added regression harness command `BP_SLZR.RunRegressionSuite` (export/validate/curve-audit orchestration) plus dedicated agent context/runbook/log docs (`AGENTS.md`, `REGRESSION_HARNESS.md`, `REGRESSION_LOG.md`) to standardize repeatable evidence-based validation for future AI sessions.
- 2026-02-18: Verified `BP_SLZR.RunRegressionSuite` end-to-end on existing batch (`skipExport=true`) and recorded artifacts (`BP_SLZR_RegressionRun_20260218_123239.json`, `BP_SLZR_ValidationReport_20260218_123239.json`, `BP_SLZR_AnimCurveAudit_20260218_123239.json`) with `overallPass=true` in validation.
- 2026-02-18: Added baseline-driven regression gates (`REGRESSION_BASELINE.json`) and upgraded `BP_SLZR.RunRegressionSuite` to emit explicit `suitePass`/`failures` by validating required gates and minimum metric floors against generated validation + curve-audit reports.
- 2026-02-18: Added host-side regression launcher `Scripts/Run-RegressionSuite.ps1` with timeout/process-lifecycle control and verified end-to-end run artifacts (`BP_SLZR_RegressionRun_20260218_142155.json`) plus automatic `UnrealEditor-Cmd` shutdown after report detection.
- 2026-02-18: Expanded converter validation with networking/replication shape gates (`variableReplicationShape`, `functionNetworkShape`) and metrics (`variablesWithReplicationShape`, `functionsWithNetworkShape`, RPC/rep-notify counts); baseline-gated regression rerun passed via wrapper (`BP_SLZR_RegressionRun_20260218_145951.json`, validation report `BP_SLZR_ValidationReport_20260218_145951.json`, `suitePass=true`).
- 2026-02-18: Applied persistent priority mandate to docs and backlog flow: normalize all CXX/CR rows to `todo` with explicit "left off" context, and keep AI-first C++ conversion fidelity as primary over editor-heavy animation authoring tasks.
- 2026-02-18: Expanded dependency closure with explicit macro linkage fields (`macroGraphPaths`, `macroBlueprintPaths`) plus validator gate/metrics (`macroDependencyClosureShape`), then validated on fresh full export batch `BP_SLZR_All_20260218_7` (`exportsWithMacroGraphPaths=164`, `macroGraphPathsTotal=510`, `macroBlueprintPathsTotal=266`, report `BP_SLZR_ValidationReport_20260218_165309.json`).
- 2026-02-18: Added `compilerIRFallback` export payload (unsupported-node snapshot + bytecode-backed function manifest) and validator gate/metrics (`compilerIRFallbackShape`), then validated on `BP_SLZR_All_20260218_7` (`exportsWithCompilerIRFallbackShape=485`, `exportsWithBytecodeFallback=205`, `compilerIRFallbackBytecodeFunctionCountTotal=841`, report `BP_SLZR_ValidationReport_20260218_165309.json`).
- 2026-02-18: Aligned gameplay-tag validation metrics to the export key `tagMetadata` (with backward-compatible legacy `metadata` fallback) in `BlueprintExtractorCommands`; attempted wrapper-based rerun is currently blocked by ShaderCompileWorker failure (`Failed to load module .../ShaderConductor/Win64/dxil.dll`) before suite-report emission (see `Saved/Logs/LyraStarterGame.log`).
- 2026-02-19: Fixed ShaderCompileWorker `dxil.dll` crash by adding `-nullrhi` to `Scripts/Run-RegressionSuite.ps1` argument string; confirmed harness runs to completion on skip-export baseline run (`suitePass=true`, report `BP_SLZR_RegressionRun_20260219_122828.json`) — stale binary produced `gameplayTagsWithSourceGraphMetadata=0` (binary from 16:58, validator source modified 18:25).
- 2026-02-19: Rebuilt `LyraEditor Win64 Development` (4 incremental actions, 35 s) to compile `BlueprintExtractorCommands.cpp` gameplay-tag validator alignment fix; ran fresh full export regression on `BP_SLZR_All_20260219_124240` confirming CR-006 metrics: `gameplayTagsWithSourceGraphMetadata=24`, `gameplayTagsWithSourceNodeTypeMetadata=24`, `gameplayTagsTotal=29` — all 14 suite gates pass, `suitePass=true` (report `BP_SLZR_ValidationReport_20260219_124249.json`).
- 2026-02-19: Tasks 33-50 — Implemented full meta-enrichment handlers for 25 K2Node types covering all high-frequency partially-supported node types in the Lyra corpus: `K2Node_CallMaterialParameterCollectionFunction`, `K2Node_GetSubsystem`/`GetEngineSubsystem`/`GetSubsystemFromPC`, `K2Node_BaseAsyncTask` subclasses (LatentAbilityCall/GameplayTaskCall/AsyncAction/PlayMontage/LoadAssetClass), `K2Node_AddComponent`, `K2Node_SetFieldsInStruct`, `K2Node_AsyncAction`, `K2Node_BaseMCDelegate` (Add/Remove/Assign), `K2Node_Message`, `K2Node_GenericCreateObject`/`CreateWidget`, `K2Node_FormatText`, `K2Node_InputKey`, `K2Node_MathExpression`, `K2Node_EnhancedInputAction`, `K2Node_AssignmentStatement`, `K2Node_TemporaryVariable`, `K2Node_PropertyAccess`, `K2Node_GetDataTableRow`, `K2Node_GetEnumeratorName`. Added 5 new Build.cs dependencies (`GameplayAbilitiesEditor`, `GameplayTasksEditor`, `InputBlueprintNodes`, `EnhancedInput`, `InputCore`) and 19 validator metric counters. Validated in regression run `BP_SLZR_RegressionRun_20260219_234159.json` (`suitePass=true`, `overallPass=true`, all 14 gates pass, `exportsWithCompilerIRFallbackShape=485`).
- 2026-02-20: CR-033 — Removed all snake_case alias keys from the `coverage` object in all 4 serialization sites (legacy structuredGraphs, StructuredGraphsExt override, fallback, AnimBlueprint); normalized node-type map key to `nodeTypeCounts`; canonical schema is now `{totalNodeCount, nodeTypeCounts, unsupportedNodeTypes, partiallySupportedNodeTypes}` plus anim-specific fields. All 14 regression gates pass post-change (`suitePass=True`, run 20260220_104354).
- 2026-02-20: Task 51 — Promoted all 11 remaining partially-supported K2Node types to fully supported: `K2Node_AsyncAction_ListenForGameplayMessages`, `K2Node_GetEnumeratorNameAsString`, `K2Node_ClearDelegate`, `K2Node_MultiGate`, `K2Node_ConvertAsset`, `K2Node_AnimNodeReference`, `K2Node_InstancedStruct`, `K2Node_CastByteToEnum`, `K2Node_PlayMontage`, `K2Node_GetInputAxisKeyValue`, `K2Node_LoadAssetClass`. Added exclusion guards in node support building loop for `EdGraphNode_Comment` (editor-only decoration) and `AnimGraphNode_*` prefix (handled by separate anim schema path) — eliminated all 164 false-positive `exportsWithUnsupportedNodeFallback` reports. Added K2Node_MultiGate handler (reads IsRandom/Loop/StartIndex pin defaults via `FindPin()`; counts output exec pins); note: `GetIsRandomPin()` etc. are not `BLUEPRINTGRAPH_API` and cannot link across DLL boundaries — workaround uses `FindPin()` + manual pin iteration inline. Validated in regression run `BP_SLZR_RegressionRun_20260220_103228.json` (`suitePass=true`, `overallPass=true`, `exportsWithUnsupportedNodeFallback=0`, `nodesWithMultiGate=1`). Updated `REGRESSION_BASELINE.json` with `nodesWithMultiGate=1` threshold.
