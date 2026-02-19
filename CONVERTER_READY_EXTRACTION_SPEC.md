# BlueprintSerializer Converter-Ready Extraction Spec

This document defines the stricter target for feeding BlueprintSerializer output into an automated Blueprint->C++ converter.

Use this with `FULL_EXTRACTION_DEFINITION.md`:
- `FULL_EXTRACTION_DEFINITION.md` = extraction schema contract.
- This file = behavior/completeness contract for conversion-quality output.
- Live progress tracker: `CONVERTER_READY_TODO.md`

## 1) What "converter-ready" means

Converter-ready means the exported data is sufficient to reconstruct:
- Blueprint class structure
- function/event behavior
- dependencies and asset/class references
- animation/control rig logic
- network/replication semantics

with minimal manual interpretation and deterministic generation.

## 1.1) C++ complete conversion (primary focus)

For this project, the primary success target is complete conversion of gameplay/class logic that is typically implemented in C++.
This mandate is persistent: prioritize AI-executable C++ parity work before editor-heavy animation authoring tasks.

Primary scope (must-pass):
- class/type/signature fidelity
- graph/control-flow/data-flow fidelity for gameplay logic
- networking and replication semantics
- component/class defaults and construction behavior
- dependency/symbol closure for compile-ready generated C++

Secondary scope (non-blocking for initial C++ focus):
- deep animation extras (rich curve payload variants, notify event payload details, rig feature metadata), unless converting AnimBP logic itself is in-scope.

## 2) Required converter-grade coverage

## 2.1 Class and type model

Must capture:
- full parent class path and implemented interfaces
- all member variables with resolved types (including object/class/struct/enums and containers)
- default values with typed serialization (not only lossy strings)
- function signatures with explicit typed inputs/outputs/return
- declaration-level metadata needed for parity (`UPROPERTY`/`UFUNCTION`/class specifier-relevant flags)
- class config identity and behavior flags used by generated C++ (`classConfigName`, `classConfigFlags`)

## 2.2 Function and graph behavior

Must capture:
- per-graph node IR with stable IDs, typed pins, and deterministic edge lists
- explicit control-flow and data-flow edges
- callable symbol resolution (`function owner path`, member name, call kind)
- event semantics (engine event, custom event, dispatcher/delegate usage)
- macro/timeline/latent/async behavior as first-class data (not name-only)
- function-local variable declarations/defaults (not just member fields)
- an explicit unsupported-node / partial-support report for fail-fast conversion

## 2.3 Runtime and networking semantics

Must capture:
- RPC flags (Server/Client/NetMulticast), reliability, validation mode
- variable replication details (`RepNotify` function, replication condition)
- authority/role checks and network-relevant branch points when represented in graph metadata

## 2.4 Components and construction behavior

Must capture:
- actual component parent-child hierarchy
- component defaults as values (not only property type names)
- component asset references/material/skeletal/static mesh bindings
- construction-script behavior graph
- inheritable component overrides from parent Blueprints
- parent/override component template identity and per-property override deltas

## 2.5 Dependencies and closure

Must capture:
- resolved dependency closure for classes, structs, enums, interfaces, assets, and control rigs
- enough symbol identity for codegen includes/module dependencies
- include hints should prefer reflected header metadata (`nativeIncludeHints`) with fallback symbolic hints (`includeHints`)

## 2.6 Animation and control rig

Must capture:
- complete AnimGraph state machines/transitions/rules/pin links
- animation assets reachable from nodes/rules/layers (with notifies/curves/sections/markers)
- control rig linkage plus rig details needed to reproduce logic surfaces
- control-rig hierarchy detail payload (`controls`, `bones`, feature/settings maps) with stable shape

## 3) Current gaps in this build (as of 2026-02-18)

The build is improved, but not yet converter-perfect.

Known gaps:
- Animation asset breadth is substantially improved but not complete (latest full batch: `23` AnimBPs detected, `11` with non-zero extracted animation assets).
- ControlRig detection remains partial in latest full batch (`23` AnimBPs detected, `3` with extracted control rigs).
- Gameplay tags are best-effort from anim variable naming/type heuristics, not full tag flow extraction.
- ControlRig hierarchy detail is now partially populated (`controls`, `bones`, feature/settings maps), and heuristic `controlToBoneMap` entries exist for some rigs, but broader control-bone semantics remain sparse.
- Include/module mapping quality improved (`nativeIncludeHints`, `Default__` canonicalization), but module ownership/include fidelity still needs deeper compile-loop validation.
- Some inheritable override edge-cases still need deeper coverage validation across larger parent-chain corpora.
- Transition detail improved: `blendMode`, `priority`, `blendOutTriggerTime`, and `notifyStateIndex` now populate from transition nodes, but deeper transition event semantics still need expansion.
- Animation curve payload shape has been expanded (`curveAssetPath`, `interpolationMode`, `axisType`, `vectorValues`, `transformValues`, `affectedMaterials`, `affectedMorphTargets`), but latest full export corpus remains float-curve-only (`animationCurvesFloatType=315`, `animationCurvesVectorType=0`, `animationCurvesTransformType=0`).
- Project-wide AnimSequence curve audit confirms corpus limitation for non-float curve validation (`animSequenceAssetsTotal=584`, `sequencesWithTransformCurves=0`, `sequencesWithMaterialCurves=0`, `sequencesWithMorphCurves=0`, report: `Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_20260218_1.json`).
- Material/morph curve linkage fields are emitted in schema shape but unpopulated in current project corpus (`animationCurvesWithAffectedMaterials=0`, `animationCurvesWithAffectedMorphTargets=0`).
- Anim notify payload fields (`trackName`, `triggeredEventName`, `eventParameters`) are now populated and exercised at scale once anim assets are discovered; remaining gap is deeper semantic payload mapping beyond reflected property-key extraction.
- Networking/replication schema shape is now enforced by validation gates (`variableReplicationShape`, `functionNetworkShape`), but current corpus still lacks non-default RPC/replication semantic usage (`functionsWithAnyNetworkSemantic=0`, `variablesReplicated=0`).
- Compiler fallback payload is now emitted (`compilerIRFallback` with unsupported-node snapshot + bytecode-backed function manifest) and validation-gated, but deeper node-to-bytecode traceability is still pending for truly lossless hard-node lowering.

Minor/polish gaps (still worth tracking):
- Some output surfaces use mixed naming conventions/schema shapes (increases parser branching).
- Legacy/active data-model surfaces should be consolidated to avoid long-term schema drift.
- External macro dependency closure shape is explicitly enforced (`macroGraphPaths`, `macroBlueprintPaths`, `macroDependencyClosureShape`) and now observed in corpus metrics, but downstream converter wiring that consumes these closure paths still needs end-to-end codegen validation.

## 4) Priority work to reach converter-ready

1. Implement compiler IR/bytecode fallback for hard-to-lower nodes (`CR-035`).
2. Add explicit external macro dependency closure wiring for cross-blueprint conversion correctness (`CR-036`).
3. Replace gameplay-tag heuristics with richer tag-flow extraction (`CR-006`).
4. Deepen include/module ownership fidelity for compile-ready generated output (`CR-014`, `CR-025`, `CR-028`).
5. Expand non-default networking/replication semantic coverage now that schema-shape gates are enforced (`CR-008`).
6. Continue animation/control-rig depth only after primary C++ mandate items above, or when AnimBP conversion is explicitly in-scope.

## 5) Acceptance gates for converter-ready status

A run is converter-ready only when all pass:
- Build passes for target engine.
- Schema contract passes (`FULL_EXTRACTION_DEFINITION.md`).
- Converter-grade fields above are populated (not default/placeholder) for the validation corpus.
- Validation corpus re-generation has no critical semantic gaps for target converter.
- Automated gate report passes for the batch (`BP_SLZR.ValidateConverterReady` overall pass + gate-level pass).
