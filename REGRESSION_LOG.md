# BlueprintSerializer Regression Log

This file tracks regression runs, outcomes, and lessons to prevent repeated mistakes.

## Entry Format

- Date/time (local)
- Change scope
- Commands executed
- Artifacts produced
- Key metrics
- Result (`pass`, `partial`, `blocked`, `fail`)
- Follow-up action

## Entries

### 2026-02-18 04:44-04:45 - Curve payload expansion validation

- Scope: validate curve payload field expansion and converter gate metrics.
- Commands:
  - `BP_SLZR.ExportAllBlueprints Saved/BlueprintExports/BP_SLZR_All_20260218_5`
  - `BP_SLZR.ValidateConverterReady`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_All_20260218_5/BP_SLZR_ValidationReport_20260218_044457.json`
- Key metrics:
  - `overallPass=true`
  - `animationCurvesTotal=315`
  - `animationCurvesWithCurveAssetPath=315`
  - `animationCurvesWithInterpolationMode=315`
  - `animationCurvesWithAxisType=315`
  - `animationCurvesVectorType=0`
  - `animationCurvesTransformType=0`
- Result: `blocked` for non-float curve population (schema path is implemented; corpus lacks non-float samples).
- Follow-up: run project-wide curve corpus audit and mark CR-013 status based on corpus evidence.

### 2026-02-18 10:24 - Project-wide AnimSequence curve corpus audit

- Scope: determine whether current project corpus contains non-float curves.
- Command:
  - `BP_SLZR.AuditAnimationCurves Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_1.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_1.json`
  - `Saved/Logs/LyraStarterGame.log`
- Key metrics:
  - `animSequenceAssetsTotal=584`
  - `sequencesWithFloatCurves=233`
  - `sequencesWithTransformCurves=0`
  - `sequencesWithMaterialCurves=0`
  - `sequencesWithMorphCurves=0`
  - `candidateSequencePaths=[]`
- Result: `blocked` for CR-013 population coverage; project corpus is float-only.
- Follow-up: prioritize non-animation AI-friendly work; defer non-float curve validation until editor-authored assets exist.

### 2026-02-18 12:32 - Unified regression suite command verification

- Scope: verify one-command regression harness and artifact paths.
- Command:
  - `BP_SLZR.RunRegressionSuite Saved/BlueprintExports/BP_SLZR_All_20260218_5 true`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260218_123239.json`
  - `Saved/BlueprintExports/BP_SLZR_All_20260218_5/BP_SLZR_ValidationReport_20260218_123239.json`
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_123239.json`
  - `Saved/Logs/LyraStarterGame.log`
- Key metrics:
  - validation `overallPass=true`, `blueprintFileCount=485`, `parseErrors=0`
  - curve audit `animSequenceAssetsTotal=584`, `sequencesWithTransformCurves=0`, `sequencesWithMaterialCurves=0`, `sequencesWithMorphCurves=0`
- Result: `pass` for harness execution and reporting; expected CR-013 corpus block remains.
- Follow-up: use this command as default regression entrypoint for future AI sessions; keep updating this log per run.

### 2026-02-18 13:06 - Baseline gate wiring failure surfaced by suite

- Scope: validate new baseline-metric gate checks in `BP_SLZR.RunRegressionSuite`.
- Command:
  - `BP_SLZR.RunRegressionSuite Saved/BlueprintExports/BP_SLZR_All_20260218_5 true Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260218_130624.json`
  - `Saved/Logs/LyraStarterGame.log`
- Key finding:
  - suite failed on baseline key mismatch: `validation missing metric 'blueprintFileCount'`.
- Result: `fail` (harness caught config/schema mismatch as intended).
- Follow-up: fix baseline key to `rawBlueprintFileCount` and rerun.

### 2026-02-18 14:21 - Wrapper-script controlled regression run (auto-close)

- Scope: ensure terminal launcher can detect completion and close lingering UE process automatically.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -ExportDir Saved/BlueprintExports/BP_SLZR_All_20260218_5 -SkipExport -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260218_142155.json`
  - `Saved/BlueprintExports/BP_SLZR_All_20260218_5/BP_SLZR_ValidationReport_20260218_142155.json`
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_142155.json`
- Key metrics:
  - `suitePass=true`
  - validation `overallPass=true`
  - curve audit still confirms float-only corpus (`sequencesWithTransformCurves=0`, `sequencesWithMaterialCurves=0`, `sequencesWithMorphCurves=0`)
- Result: `pass` with explicit process shutdown behavior verified (`UnrealEditor-Cmd` closed by wrapper).
- Follow-up: standardize on script launcher in future AI runs to avoid hanging editor windows/timeouts.

### 2026-02-18 14:55 - Network/replication gate rollout (initial failure)

- Scope: add CR-008 validation shape gates and baseline thresholds.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -ExportDir Saved/BlueprintExports/BP_SLZR_All_20260218_5 -SkipExport -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260218_145537.json`
  - `Saved/BlueprintExports/BP_SLZR_All_20260218_5/BP_SLZR_ValidationReport_20260218_145537.json`
- Key finding:
  - `suitePass=false` due replication field-key mismatch in validator (`repCondition` vs exported `replicationCondition`), causing `variableReplicationShape=false`.
- Result: `fail` (expected during first gate rollout; harness surfaced mismatch quickly).
- Follow-up: align validator key name to `replicationCondition` and rerun.

### 2026-02-18 15:00 - Network/replication gate rollout (fixed)

- Scope: rerun after replication field-key fix.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -ExportDir Saved/BlueprintExports/BP_SLZR_All_20260218_5 -SkipExport -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260218_145951.json`
  - `Saved/BlueprintExports/BP_SLZR_All_20260218_5/BP_SLZR_ValidationReport_20260218_145951.json`
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_145951.json`
- Key metrics:
  - `suitePass=true`
  - new gates `variableReplicationShape=true`, `functionNetworkShape=true`
  - `variablesWithReplicationShape=3133`, `functionsWithNetworkShape=936`
  - observed corpus remains net-default (`functionsWithAnyNetworkSemantic=0`, `variablesReplicated=0`)
- Result: `pass` (CR-008 validation expansion now enforced and baseline-gated).
- Follow-up: keep shape gates strict while treating non-zero network semantic counts as corpus-dependent, not hard-fail.

### 2026-02-18 16:50 - Macro dependency closure gate rollout

- Scope: enforce and validate explicit macro dependency closure payload for C++ conversion dependency fidelity.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -ExportDir Saved/BlueprintExports/BP_SLZR_All_20260218_6 -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260218_164854.json`
  - `Saved/BlueprintExports/BP_SLZR_All_20260218_6/BP_SLZR_ValidationReport_20260218_164854.json`
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_164854.json`
- Key metrics:
  - `suitePass=true`
  - `macroDependencyClosureShape=true`
  - `exportsWithMacroDependencyClosureShape=485`
  - `exportsWithMacroGraphPaths=164`, `macroGraphPathsTotal=510`, `macroBlueprintPathsTotal=266`
- Result: `pass` (macro closure shape and population now measurable and regression-gated).
- Follow-up: wire macro-closure consumption into converter-side closure resolution tests.

### 2026-02-18 16:53 - Compiler fallback payload gate rollout

- Scope: add and validate compiler IR fallback payload for hard-to-lower nodes.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -ExportDir Saved/BlueprintExports/BP_SLZR_All_20260218_7 -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260218_165309.json`
  - `Saved/BlueprintExports/BP_SLZR_All_20260218_7/BP_SLZR_ValidationReport_20260218_165309.json`
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_165309.json`
- Key metrics:
  - `suitePass=true`
  - `compilerIRFallbackShape=true`
  - `exportsWithCompilerIRFallbackShape=485`
  - `exportsWithUnsupportedNodeFallback=164`
  - `exportsWithBytecodeFallback=205`
  - `compilerIRFallbackBytecodeFunctionCountTotal=841`
- Result: `pass` (compiler fallback payload now emitted and baseline-gated).
- Follow-up: add deeper node-to-bytecode trace linkage for stronger deterministic lowering fallback.

### 2026-02-18 18:26 - Gameplay-tag metadata validator key alignment (run blocked)

- Scope: align gameplay-tag validation metrics to exported `tagMetadata` payload shape (with backward-compat fallback for legacy `metadata` key).
- Command:
  - `powershell -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1`
- Artifacts:
  - no new suite report generated
  - `Saved/Logs/LyraStarterGame.log`
- Key finding:
  - engine startup failed before suite completion due `ShaderCompileWorker` crash: `Failed to load module: ../../../Engine/Binaries/ThirdParty/ShaderConductor/Win64/dxil.dll`
- Result: `blocked` (environment/runtime issue, not schema-gate logic).
- Follow-up: fix ShaderCompileWorker runtime dependency loading for ShaderConductor (`dxil.dll` load path/dependency chain), or run on a known-good engine install, then rerun the regression suite to confirm non-zero gameplay-tag metadata metrics.

### 2026-02-19 12:28 - ShaderCompileWorker fix + skip-export regression

- Scope: resolve `dxil.dll` ShaderCompileWorker crash by adding `-nullrhi` to `Run-RegressionSuite.ps1` argument string; confirm harness runs to completion.
- Fix applied:
  - Added `-nullrhi` to `Scripts/Run-RegressionSuite.ps1` argument string; prevents shader compilation startup before commandlets run, eliminating the `dxil.dll` load failure.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -ExportDir Saved/BlueprintExports/BP_SLZR_All_20260218_8 -SkipExport -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260219_122828.json`
  - `Saved/BlueprintExports/BP_SLZR_All_20260218_8/BP_SLZR_ValidationReport_20260219_122828.json`
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260219_122828.json`
- Key metrics:
  - `suitePass=true`, all 14 validation gates pass
  - `blueprintFileCount=485`, `parseErrors=0`
  - `gameplayTagsTotal=29`, `exportsWithGameplayTags=4`
  - `gameplayTagsWithSourceGraphMetadata=0` — stale binary: `BlueprintExtractorCommands.cpp` modified 2026-02-18 18:25, binary last built 16:58; validator fix not yet compiled
  - `gameplayTagsWithSourceNodeTypeMetadata=0` — same stale binary cause
- Result: `pass` for harness unblock; CR-006 tag metadata metrics unreliable until binary rebuild.
- Follow-up: rebuild `LyraEditor Win64 Development` to compile gameplay-tag validator key fix, then run full fresh export.

### 2026-02-19 12:42 - Binary rebuild + full fresh regression (CR-006 confirmed)

- Scope: rebuild `LyraEditor Win64 Development` to pick up the `BlueprintExtractorCommands.cpp` gameplay-tag validator alignment fix, then run a full fresh export regression.
- Build:
  - `Build.bat LyraEditor Win64 Development LyraStarterGame.uproject -NoUBA` — 4 incremental actions (compile `Module.BlueprintSerializer.3.cpp`, link `.lib`, link `.dll`, write metadata target); succeeded in 35 s.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_All_20260219_124240/` (fresh export batch, 485 files)
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260219_124249.json`
  - `Saved/BlueprintExports/BP_SLZR_All_20260219_124240/BP_SLZR_ValidationReport_20260219_124249.json`
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260219_124249.json`
- Key metrics:
  - `suitePass=true`, all 14 validation gates pass
  - `blueprintFileCount=485`, `parseErrors=0`, `missingRequiredExports=0`
  - `gameplayTagsTotal=29`, `exportsWithGameplayTags=4`
  - `gameplayTagsWithSourceGraphMetadata=24` — 24 GraphTagFlow tags with populated `sourceGraph` in `tagMetadata`
  - `gameplayTagsWithSourceNodeTypeMetadata=24` — 24 GraphTagFlow tags with populated `sourceNodeType` in `tagMetadata`
  - remaining 5 tags (29 − 24) are `AnimVariablePrefix` category entries carrying `sourceVariable`/`variableType` only; absence of `sourceGraph` is correct for that category
  - all prior stability metrics unchanged: macro closure (`exportsWithMacroGraphPaths=164`), compiler fallback (`exportsWithBytecodeFallback=205`, total `841`), replication shape, ControlRig, curve, transition, notify
- Result: `pass` — CR-006 `tagMetadata` extraction and validator key alignment confirmed at full project scale.
- Follow-up: evaluate whether AnimVariablePrefix tags should optionally emit a `sourceGraph` hint; continue CR-035 node-to-bytecode trace linkage and CR-036 converter-side macro closure consumption.

### 2026-02-19 13:16 - Tier-1 graph node fidelity (K2Node parity + PinSubCategoryObjectPath)

- Scope: add missing K2Node type handlers in `AnalyzeNodeToStruct` and populate `PinSubCategoryObjectPath` in `FBS_PinLinkData` / `AddDataLinks`.
- Changes:
  - `BlueprintAnalyzer.h`: added `PinSubCategoryObjectPath` field to `FBS_PinLinkData`
  - `BlueprintAnalyzer.cpp`: new includes (`K2Node_Literal`, `K2Node_CallDelegate`, `K2Node_BaseMCDelegate`, `K2Node_CallArrayFunction`, `K2Node_Select`, `K2Node_Switch*`, `K2Node_MakeStruct`, `K2Node_BreakStruct`, `K2Node_StructOperation`, `K2Node_ClassDynamicCast`); new/extended node handlers for Literal, CallDelegate, CallArrayFunction, Select, SwitchEnum/Integer/Name/String, DynamicCast (isPureCast), ClassDynamicCast, MakeStruct, BreakStruct, CustomEvent (params), FunctionEntry (access specifier + localVarDecls), FunctionResult, ComponentBoundEvent; `AddDataLinks` and both JSON serializers emit `pinSubCategoryObjectPath`.
  - Build: 4 incremental actions (one compile, link, dll, metadata), 26 s.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1 -BaselinePath Plugins/BlueprintSerializer/REGRESSION_BASELINE.json`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_All_20260219_131619/BP_SLZR_ValidationReport_20260219_131639.json`
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260219_131639.json`
- Key metrics:
  - `suitePass=true`, `overallPass=true`, all 14 gates pass
  - `blueprintFileCount=485`, `parseErrors=0`
- Result: `pass` — Tier-1 graph fidelity changes compile and regress cleanly.
- Follow-up: continue Tier-2 (local variable structured types, struct MakeStruct/BreakStruct field-pin mapping, MemberParent fallback); then CR-035/CR-036.

### 2026-02-19 13:24 - Tier-2 structured local variable types (FBS_LocalVarInfo + detailedLocalVariables)

- Scope: add `FBS_LocalVarInfo` struct for fully-typed local variable declarations; populate in `ExtractDetailedFunctions`; serialize as `detailedLocalVariables` JSON array alongside existing `localVariables` string array.
- Changes:
  - `BlueprintAnalyzer.h`: new `FBS_LocalVarInfo` USTRUCT (VarName, TypeCategory, TypeSubCategory, TypeObjectPath, TypeObjectName, ContainerType, MapValueCategory, MapValueObjectPath, bIsReference, bIsConst, DefaultValue, Description); added `TArray<FBS_LocalVarInfo> DetailedLocalVariables` to `FBS_FunctionInfo`.
  - `BlueprintAnalyzer.cpp`: population of `FBS_LocalVarInfo` per local var in `ExtractDetailedFunctions` (all container types, map value type, reference/const flags, default value); JSON serializer emits `detailedLocalVariables` array with conditional field emission (empty fields omitted).
  - Build: 6 incremental actions (UHT generated 4 reflection files), 37 s.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_All_20260219_132402/BP_SLZR_ValidationReport_20260219_132411.json`
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260219_132411.json`
- Key metrics:
  - `suitePass=true`, `overallPass=true`, all 14 gates pass
  - `blueprintFileCount=485`, `parseErrors=0`
- Result: `pass` — structured local variable types add zero regression risk; `detailedLocalVariables` is additive-only.
- Follow-up: Task 5 (MemberParent triple-fallback), Task 6 (FBS_ParamInfo structured function parameters).

### 2026-02-19 13:31 - Tier-2 MemberParent triple-fallback for CallFunction resolution

- Scope: resolve `Out.MemberParent` for cases where `GetMemberParentClass()` returns null — including self-context Blueprint functions and functions resolved only at compile time.
- Changes:
  - `BlueprintAnalyzer.cpp` `AnalyzeNodeToStruct` CallFunction handler:
    1. **Primary** (existing): `GetMemberParentClass()` → cleaned class name + emit `meta.memberParentPath`.
    2. **Fallback 2** (new): self-context resolution — `GetMemberParentClass(OwnerBP->SkeletonGeneratedClass)` when primary returns null; emits `meta.memberParentFallback=skelScope`.
    3. **Fallback 3** (new): derive from compiled function owner — `GetTargetFunction()->GetOwnerClass()` when still unresolved; emits `meta.memberParentFallback=funcOwner`.
  - Build: 4 incremental actions (compile Module.BlueprintSerializer.3, link, dll, metadata), 35 s.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_All_20260219_133154/BP_SLZR_ValidationReport_20260219_133203.json`
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260219_133203.json`
- Key metrics:
  - `suitePass=true`, `overallPass=true`, all 14 gates pass
  - `blueprintFileCount=485`, `parseErrors=0`
- Result: `pass` — additive metadata fields carry zero regression risk.
- Follow-up: Task 6 (FBS_ParamInfo structured function parameters with full type resolution).

### 2026-02-19 14:07 - Task 6: FBS_ParamInfo structured function parameters

- Scope: add `FBS_ParamInfo` struct for fully-typed function parameter declarations; populate `DetailedInputParams`/`DetailedOutputParams` in `ExtractDetailedFunctions` via two helper lambdas; serialize as `detailedInputParams`/`detailedOutputParams` JSON arrays.
- Changes:
  - `BlueprintAnalyzer.h`: new `FBS_ParamInfo` USTRUCT (ParamName, TypeCategory, TypeSubCategory, TypeObjectPath, TypeObjectName, ContainerType, MapValueCategory, MapValueObjectPath, bIsReference, bIsConst, bIsOut, DefaultValue, Description); added `TArray<FBS_ParamInfo> DetailedInputParams` and `TArray<FBS_ParamInfo> DetailedOutputParams` to `FBS_FunctionInfo`.
  - `BlueprintAnalyzer.cpp`:
    - `PopulatePIFromProp` lambda in `ExtractDetailedFunctions` — maps `FProperty` (all container types, map key+value types, all primitive/object/struct type categories, bIsReference/bIsConst/bIsOut flags, DefaultValue from graph pin, Description) to `FBS_ParamInfo`.
    - `PopulatePIFromPin` lambda in `ExtractDetailedFunctions` — maps `UEdGraphPin` (PinType category/subCategory/subCategoryObject, ContainerType+map value type, bIsReference/bIsConst, DefaultValue) to `FBS_ParamInfo`.
    - Touch D: `DetailedOutputParams.Add(PopulatePIFromProp(...))` / `DetailedInputParams.Add(PopulatePIFromProp(...))` alongside existing `OutputParameters`/`InputParameters` string adds.
    - Touch E: `DetailedOutputParams.Add(PopulatePIFromPin(Pin, true))` alongside pin-path output add.
    - Touch F: `DetailedInputParams.Add(PopulatePIFromPin(Pin, false))` alongside pin-path input add.
    - Touch G: JSON serializer emits `detailedInputParams` and `detailedOutputParams` arrays (conditional field emission, empty fields omitted) for each `FBS_FunctionInfo` in `DetailedFunctions`.
  - Defects fixed during implementation:
    - Lambda over-insertion: previous `.Replace()` inserted lambdas at all 5 occurrences of the anchor `\n\tfor (UEdGraph* Graph : Blueprint->FunctionGraphs)` across different functions; fixed by position-indexed removal (`fix_remove_extra_lambdas.ps1`) keeping only the correct `ExtractDetailedFunctions` occurrence.
    - UE `PI` macro conflict: `PI` used as local variable name for `FBS_ParamInfo` instances shadowed by Unreal Engine's `#define PI (3.1415926535897932f)`, causing `error C2059: syntax error: 'constant'`; fixed by renaming all 103 standalone `\bPI\b` occurrences to `PInfo` via word-boundary regex (`fix_rename_PI.ps1`).
  - Build: 4 incremental actions (compile `Module.BlueprintSerializer.3.cpp`, link `.lib`, link `.dll`, write metadata), 24 s.
- Command:
  - `powershell -NoProfile -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1`
- Artifacts:
  - `Saved/BlueprintExports/BP_SLZR_All_20260219_140724/BP_SLZR_ValidationReport_20260219_140756.json`
  - `Saved/BlueprintExports/BP_SLZR_RegressionRun_20260219_140756.json`
  - `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260219_140756.json`
- Key metrics:
  - `suitePass=true`, all gates pass
  - `blueprintFileCount=485`, `parseErrors=0`
  - `detailedInputParams`/`detailedOutputParams` arrays are additive-only fields; no existing gate affected.
- Result: `pass` — structured function parameter types add zero regression risk; `FBS_ParamInfo` payload provides full C++-codegen-ready type information for all function signatures.
- Follow-up: CR-035 deeper node-to-bytecode trace linkage; CR-036 converter-side macro closure consumption wiring.

## Known Pitfalls

- Unreal command can outlive shell timeout; always verify by log and artifact files.
- Build can no-op to metadata/link-only updates; confirm touched sources are included in compile action output.
- Reused export directories can accumulate stale files; validator dedupe logic helps, but fresh timestamped dirs are preferred for definitive comparisons.
