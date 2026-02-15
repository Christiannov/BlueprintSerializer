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

## 2.5 Dependencies and closure

Must capture:
- resolved dependency closure for classes, structs, enums, interfaces, assets, and control rigs
- enough symbol identity for codegen includes/module dependencies

## 2.6 Animation and control rig

Must capture:
- complete AnimGraph state machines/transitions/rules/pin links
- animation assets reachable from nodes/rules/layers (with notifies/curves/sections/markers)
- control rig linkage plus rig details needed to reproduce logic surfaces

## 3) Current gaps in this build (as of 2026-02-14)

The build is improved, but not yet converter-perfect.

Known gaps:
- Animation asset breadth is low in latest batch (20 AnimBPs detected, assets extracted for 1).
- ControlRig detection is partial in latest batch (20 AnimBPs detected, control rigs extracted for 3).
- Gameplay tags are best-effort from anim variable naming/type heuristics, not full tag flow extraction.
- ControlRig detail fields are emitted but currently unpopulated (`controls`, `bones`, `controlToBoneMap`, feature maps).
- Component extraction is incomplete (`parentComponent` is hardcoded `Unknown`; property values use type names only).
- Component-level `assetReferences` are currently not populated.
- Function metadata is partial (`returnType` is emitted but not populated; RPC/reliability flags are not exported).
- Delegate/event-dispatcher signature graphs are not exported (`DelegateSignatureGraphs` not covered).
- Timeline templates are not exported (only timeline node name metadata is captured).
- Function-local variable declarations/defaults are not exported.
- Type model is still lossy in detailed function/variable output (insufficient for deterministic type reconstruction).
- Class default object (CDO) property deltas are not exported as a class-default layer.
- Symbol identity is inconsistent (several fields use short names instead of fully-qualified paths).
- Graph extraction currently iterates only Ubergraph/Function/Macro arrays, not all editable graph categories.
- Declaration-specifier metadata is incomplete for parity-grade header generation.
- Transition detail is partial (only some transition fields are populated; several remain defaults unless added).
- Animation curves currently extract float curves only; vector/transform curve payloads are not populated.
- Anim notify extended fields (`triggeredEventName`, `eventParameters`) are emitted but currently not populated.
- Optional compiler-IR/bytecode fallback export is not available for hard-to-lower node cases.

Minor/polish gaps (still worth tracking):
- Blueprint namespace/imported-namespace metadata is not exported.
- Construction-script role is not explicitly tagged in graph metadata.
- Some output surfaces use mixed naming conventions/schema shapes (increases parser branching).
- Top-level batch ordering is not explicitly normalized for full diff determinism.
- Legacy/active data-model surfaces should be consolidated to avoid long-term schema drift.
- External macro dependency wiring should be made explicit for deterministic cross-blueprint conversion.

## 4) Priority work to reach converter-ready

1. Raise Anim asset/control-rig discovery coverage (sanity-set based gates).
2. Complete function/network metadata export (return type + RPC semantics).
3. Implement real component hierarchy/value extraction.
4. Fill ControlRig detail model (`controls`, `bones`, mappings, features).
5. Expand notify/curve extraction depth for animation payload fidelity.

## 5) Acceptance gates for converter-ready status

A run is converter-ready only when all pass:
- Build passes for target engine.
- Schema contract passes (`FULL_EXTRACTION_DEFINITION.md`).
- Converter-grade fields above are populated (not default/placeholder) for the validation corpus.
- Validation corpus re-generation has no critical semantic gaps for target converter.
