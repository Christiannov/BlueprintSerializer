# BlueprintSerializer Converter-Ready TODO

Live tracker for closing gaps in `CONVERTER_READY_EXTRACTION_SPEC.md`.

Last updated: 2026-02-14
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

## Work items

| ID | Area | Task | Status | Notes |
|---|---|---|---|---|
| CR-001 | Schema | Keep required stable keys always emitted for all exports | done | `structuredGraphs`, `coverage`, `graphsSummary` are stable |
| CR-002 | Anim vars | Resolve AnimBP variable defaults robustly | in_progress | `116/117` populated; finish edge cases |
| CR-003 | Anim assets | Increase animation asset discovery coverage from graph/rule/layer/property paths | in_progress | current `1/20` AnimBPs with assets |
| CR-004 | ControlRig linkage | Discover and extract referenced ControlRigs from AnimBP nodes/properties | in_progress | current `3/20` AnimBPs with rigs |
| CR-005 | ControlRig detail model | Populate `controls`, `bones`, `controlToBoneMap`, feature fields | todo | fields exist but are mostly empty |
| CR-006 | Gameplay tags | Replace name/type heuristics with richer tag flow extraction | todo | currently best-effort only |
| CR-007 | Function signatures | Populate explicit return type + richer typed parameter metadata | todo | `returnType` currently empty in sampled batch |
| CR-008 | Networking semantics | Export RPC kind/reliability/validation + RepNotify linkage | todo | required for converter-grade parity |
| CR-009 | Components | Extract real component hierarchy and actual default values | todo | `parentComponent` currently unresolved |
| CR-010 | Component deps | Populate component-level asset/class references | todo | currently empty in sampled batch |
| CR-011 | Transition fidelity | Populate full transition fields (blend mode, priority, notify index, etc.) | todo | some fields remain defaults |
| CR-012 | Notify fidelity | Populate track/event payload (`trackName`, `eventParameters`, etc.) | todo | currently minimal notify payload |
| CR-013 | Curve fidelity | Add vector/transform/material/morph curve extraction where available | todo | currently float-curve path only |
| CR-014 | Dependency closure | Emit converter-facing dependency closure manifest (types/modules/includes) | todo | needed for deterministic codegen |
| CR-015 | Tooling parity | Implement missing module entrypoints (`BP_SLZR.Serialize`, selected serialize, context generation) | todo | `BlueprintSerializerModule.cpp` stubs + `UBlueprintSerializerBlueprintLibrary::GenerateLLMContext()` not implemented |
| CR-016 | Validation gates | Automate converter-ready gate checks and publish pass/fail report per run | todo | currently manual ad-hoc checks |
| CR-017 | Legacy path cleanup | Replace/remove `AnalyzeNodeDetails` temporary stub path to avoid partial legacy behavior | todo | `BlueprintAnalyzer.cpp` contains disabled legacy node-analysis stub |
| CR-018 | Command surface consistency | Unify command entrypoints so all public commands use the same extraction/export pipeline | todo | currently mixed command surfaces (`BP_SLZR.Serialize` vs extractor command set) |
| CR-019 | Delegate model | Extract dispatcher declarations + delegate signature graphs as converter IR | todo | `DelegateSignatureGraphs` are not currently exported |
| CR-020 | Timeline model | Extract `UTimelineTemplate` tracks/keys/events/length/loop settings (not only node name) | todo | current output captures timeline node name only |
| CR-021 | Function locals | Extract function-local variable declarations/defaults/scopes from function entry metadata | todo | currently member variables only |
| CR-022 | Type fidelity | Export full variable type model (container kind, key/value type, object/class path) for member vars/functions | todo | current detailed type strings are lossy |
| CR-023 | Class/component overrides | Export inheritable component overrides and class-level config flags relevant to generated C++ | todo | needed for parent/child parity |
| CR-024 | Class defaults | Export class default object (CDO) property delta for converter-visible gameplay settings | todo | variable defaults alone are insufficient for full constructor parity |
| CR-025 | Symbol identity | Export fully-qualified paths for parent classes, interfaces, types, and callable owners | todo | currently many fields use short names only |
| CR-026 | Graph coverage | Include delegate/collapsed/other relevant editable graphs beyond Ubergraph/Function/Macro | todo | extraction currently iterates only 3 graph arrays |
| CR-027 | Override semantics | Export explicit override/parent-call semantics for events and overridden functions | todo | needed for accurate super-call generation |
| CR-028 | User type schemas | Export user-defined struct/enum schema details required for C++ type generation/mapping | todo | currently only references/typestrings are exported |
| CR-029 | Declaration specifiers | Export class/property/function specifier-level metadata needed for header generation parity | todo | partial flags exist today; full declaration fidelity is not exported |

## Minor C++ parity gaps (polish)

These are smaller but still useful for high-confidence, low-friction conversion output.

| ID | Area | Task | Status | Notes |
|---|---|---|---|---|
| CR-030 | Blueprint metadata | Export `BlueprintNamespace`, imported namespaces, and additional class-level metadata used for symbol resolution | todo | currently not extracted in analyzer |
| CR-031 | Construction role tagging | Explicitly tag/identify construction-script graph role in structured graph output | todo | today construction graphs appear as generic `Function` graph type |
| CR-032 | Determinism | Sort top-level analyzed Blueprint list and other non-semantic arrays for stable cross-run diffs | todo | maps are sorted; several arrays still preserve source order |
| CR-033 | Schema consistency | Normalize key casing/schema shapes where currently mixed (for example coverage object variants) | todo | reduces converter parser branching |
| CR-034 | Model consolidation | Consolidate legacy vs active data model surfaces to prevent schema drift (e.g. duplicate type definitions) | todo | improves long-term maintainability and tooling correctness |
| CR-035 | Compiler IR fallback | Export optional compiled-script/bytecode metadata for nodes that cannot be deterministically lowered from graph data alone | todo | enables safer handling of uncommon/custom node behavior |
| CR-036 | Macro dependency fidelity | Explicitly resolve/link external macro instance dependencies for robust cross-blueprint conversion | todo | call-site metadata exists, but conversion pipeline needs guaranteed closure wiring |
| CR-037 | Node support matrix | Emit per-export unsupported/partially-supported node report for converter gating and fail-fast behavior | todo | prevents silent semantic loss during generation |

## Update protocol

For each completed task:

1. Change status in the table.
2. Add one changelog entry with date and evidence (file path, export sample, or metric delta).
3. If contract scope changed, update `CONVERTER_READY_EXTRACTION_SPEC.md`.

## Changelog

- 2026-02-14: Created tracker and seeded baseline metrics from `_20260214_1305` export batch.
