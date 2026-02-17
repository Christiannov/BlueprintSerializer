# BlueprintSerializer Converter-Ready TODO

Live tracker for closing gaps in `CONVERTER_READY_EXTRACTION_SPEC.md`.

Last updated: 2026-02-16
Owner: BlueprintSerializer team

## Status legend

- `done` = implemented and validated against the current validation corpus.
- `in_progress` = partially implemented or implemented without full validation.
- `todo` = not implemented yet.
- `blocked` = cannot proceed due to engine/API/project constraint.

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

## C++ complete conversion focus (primary)

This section defines the must-have subset for "things usually done in C++".
We should treat these as blocking items before calling the serializer C++-complete.

| Focus ID | Goal | Mapped work items | Status |
|---|---|---|---|
| CXX-001 | Deterministic class/function graph IR for codegen | CR-001, CR-016, CR-018, CR-026 | in_progress |
| CXX-002 | Lossless type/signature model (params/returns/containers/object paths) | CR-007, CR-022, CR-025 | in_progress |
| CXX-003 | Runtime semantics parity (RPC/replication/authority/delegates/latent/timeline) | CR-008, CR-019, CR-020 | in_progress |
| CXX-004 | Class defaults + component hierarchy/defaults/overrides parity | CR-009, CR-010, CR-023, CR-024 | in_progress |
| CXX-005 | Symbol/dependency closure for generated C++ includes/modules | CR-014, CR-025, CR-028 | in_progress |
| CXX-006 | Function implementation parity for real project patterns | CR-021, CR-027 | in_progress |
| CXX-007 | UPROPERTY/UFUNCTION/class specifier parity for generated C++ declarations | CR-023, CR-029 | in_progress |
| CXX-008 | Unsupported/custom node fallback path for correctness | CR-035, CR-037 | in_progress |

Notes:
- Animation depth items remain important for AnimBP conversion, but they are secondary to the CXX-* goals for gameplay/class conversion.

## Work items

| ID | Area | Task | Status | Notes |
|---|---|---|---|---|
| CR-001 | Schema | Keep required stable keys always emitted for all exports | done | `structuredGraphs`, `coverage`, `graphsSummary` are stable |
| CR-002 | Anim vars | Resolve AnimBP variable defaults robustly | in_progress | `116/117` populated; finish edge cases |
| CR-003 | Anim assets | Increase animation asset discovery coverage from graph/rule/layer/property paths | in_progress | current `1/20` AnimBPs with assets |
| CR-004 | ControlRig linkage | Discover and extract referenced ControlRigs from AnimBP nodes/properties | in_progress | current `3/20` AnimBPs with rigs |
| CR-005 | ControlRig detail model | Populate `controls`, `bones`, `controlToBoneMap`, feature fields | in_progress | hierarchy-driven `controls`/`bones`/feature maps now populated; `controlToBoneMap` remains sparse and needs deeper mapping logic |
| CR-006 | Gameplay tags | Replace name/type heuristics with richer tag flow extraction | todo | currently best-effort only |
| CR-007 | Function signatures | Populate explicit return type + richer typed parameter metadata | in_progress | return/object-path metadata now exported; needs wider corpus validation |
| CR-008 | Networking semantics | Export RPC kind/reliability/validation + RepNotify linkage | in_progress | network flags + RepNotify now exported; validation expansion pending |
| CR-009 | Components | Extract real component hierarchy and actual default values | in_progress | parent/root/inherited metadata + default property extraction implemented |
| CR-010 | Component deps | Populate component-level asset/class references | in_progress | component asset references now emitted; validate at scale |
| CR-011 | Transition fidelity | Populate full transition fields (blend mode, priority, notify index, etc.) | todo | some fields remain defaults |
| CR-012 | Notify fidelity | Populate track/event payload (`trackName`, `eventParameters`, etc.) | todo | currently minimal notify payload |
| CR-013 | Curve fidelity | Add vector/transform/material/morph curve extraction where available | todo | currently float-curve path only |
| CR-014 | Dependency closure | Emit converter-facing dependency closure manifest (types/modules/includes) | in_progress | emits class/struct/enum/interface/asset/control-rig/module sets plus `includeHints` and `nativeIncludeHints`; include/module fidelity still needs deeper compile-loop tuning |
| CR-015 | Tooling parity | Implement missing module entrypoints (`BP_SLZR.Serialize`, selected serialize, context generation) | done | wired module commands + implemented `GenerateLLMContext()` |
| CR-016 | Validation gates | Automate converter-ready gate checks and publish pass/fail report per run | done | `BP_SLZR.ValidateConverterReady` emits per-run JSON report with gate-level pass/fail + coverage metrics and now de-duplicates reused export directories |
| CR-017 | Legacy path cleanup | Replace/remove `AnalyzeNodeDetails` temporary stub path to avoid partial legacy behavior | todo | `BlueprintAnalyzer.cpp` contains disabled legacy node-analysis stub |
| CR-018 | Command surface consistency | Unify command entrypoints so all public commands use the same extraction/export pipeline | in_progress | major path now unified; legacy extractor command surface still exists |
| CR-019 | Delegate model | Extract dispatcher declarations + delegate signature graphs as converter IR | in_progress | delegate signatures + delegate graph coverage now exported |
| CR-020 | Timeline model | Extract `UTimelineTemplate` tracks/keys/events/length/loop settings (not only node name) | in_progress | timeline template/track/key metadata now exported |
| CR-021 | Function locals | Extract function-local variable declarations/defaults/scopes from function entry metadata | in_progress | local variable declarations now exported |
| CR-022 | Type fidelity | Export full variable type model (container kind, key/value type, object/class path) for member vars/functions | in_progress | added category/subcategory/object-path/container flags |
| CR-023 | Class/component overrides | Export inheritable component overrides and class-level config flags relevant to generated C++ | in_progress | inheritable override metadata/delta fields plus class-level `classConfigFlags` now emitted; expand override-depth edge cases |
| CR-024 | Class defaults | Export class default object (CDO) property delta for converter-visible gameplay settings | in_progress | `classDefaultValues` + `classDefaultValueDelta` now emitted; filtering/scope tuning still pending |
| CR-025 | Symbol identity | Export fully-qualified paths for parent classes, interfaces, types, and callable owners | in_progress | class/interface/type paths now exported in primary schema |
| CR-026 | Graph coverage | Include delegate/collapsed/other relevant editable graphs beyond Ubergraph/Function/Macro | in_progress | added delegate/event coverage and construction role tagging |
| CR-027 | Override semantics | Export explicit override/parent-call semantics for events and overridden functions | in_progress | `bIsOverride`/`bCallsParent` + parent-call node metadata exported |
| CR-028 | User type schemas | Export user-defined struct/enum schema details required for C++ type generation/mapping | in_progress | `userDefinedStructSchemas` / `userDefinedEnumSchemas` now emitted; richer defaults/metadata still pending |
| CR-029 | Declaration specifiers | Export class/property/function specifier-level metadata needed for header generation parity | in_progress | class-level `classSpecifiers` plus variable/function `declarationSpecifiers` now emitted; deeper UHT parity still pending |

## Minor C++ parity gaps (polish)

These are smaller but still useful for high-confidence, low-friction conversion output.

| ID | Area | Task | Status | Notes |
|---|---|---|---|---|
| CR-030 | Blueprint metadata | Export `BlueprintNamespace`, imported namespaces, and additional class-level metadata used for symbol resolution | done | namespace/imported namespace fields are now emitted |
| CR-031 | Construction role tagging | Explicitly tag/identify construction-script graph role in structured graph output | done | construction graphs tagged as `ConstructionScript` |
| CR-032 | Determinism | Sort top-level analyzed Blueprint list and other non-semantic arrays for stable cross-run diffs | done | project asset list/export arrays now sorted for stable output |
| CR-033 | Schema consistency | Normalize key casing/schema shapes where currently mixed (for example coverage object variants) | in_progress | canonical + alias coverage keys now emitted; full cleanup pending |
| CR-034 | Model consolidation | Consolidate legacy vs active data model surfaces to prevent schema drift (e.g. duplicate type definitions) | todo | improves long-term maintainability and tooling correctness |
| CR-035 | Compiler IR fallback | Export optional compiled-script/bytecode metadata for nodes that cannot be deterministically lowered from graph data alone | todo | enables safer handling of uncommon/custom node behavior |
| CR-036 | Macro dependency fidelity | Explicitly resolve/link external macro instance dependencies for robust cross-blueprint conversion | todo | call-site metadata exists, but conversion pipeline needs guaranteed closure wiring |
| CR-037 | Node support matrix | Emit per-export unsupported/partially-supported node report for converter gating and fail-fast behavior | done | `unsupportedNodeTypes`/`partiallySupportedNodeTypes` now emitted |

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
