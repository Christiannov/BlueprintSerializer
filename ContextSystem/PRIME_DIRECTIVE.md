# THE PRIME DIRECTIVE

> Build a complete, AI-consumable knowledge system for any Unreal Engine project —
> so that any AI agent can deeply understand how the project works and build on top
> of it correctly, following the project's own architecture and conventions, without
> processing billions of tokens of raw source.

---

## What This Is

The PRIME DIRECTIVE is a universal mandate for AI-assisted Unreal Engine game development.
It applies to **any** UE project — Lyra, a custom shooter, an RPG, a tool plugin, anything.

The goal: extract everything an AI needs to know about a UE project and convert it into
a knowledge system that any AI can load and immediately work fluently as a developer on
that project.

## The Pipeline

```
Phase 1: EXTRACT
  BlueprintSerializer plugin → structured JSON IR for every Blueprint in the project
  (project-agnostic, runs on any UE5 project)

Phase 2: SPEC (AI-driven)
  AI agent + Context Engineering System → reads the raw JSON exports directly,
  understands the project architecture, and produces per-Blueprint specs.
  THE SPEC IS THE COMPRESSION. Converting raw JSON → organized spec IS the
  data reduction step. No separate pre-processor needed.
  The AI works incrementally — one Blueprint at a time, tracking progress
  through git commits and checklists. It does NOT need to fit the entire
  corpus into a single context window.

Phase 3: SYNTHESIZE (future)
  All knowledge sources combined: Blueprint specs, C++ source analysis, official docs,
  community guides → queryable knowledge base for the project
```

**Phase 2 is done by AI, not scripts.** A Python script cannot understand what a system
does, why it was designed that way, or how components relate architecturally. Only an AI
reading the data with proper context can produce specs that capture intent and architecture.

**The spec IS the compression.** The raw JSON IR contains everything — coordinates, GUIDs,
pin IDs, and other structural data alongside the semantically meaningful content. The AI's
job is to read the raw data and produce specs that retain all technically relevant detail
while organizing it into a dense, readable format. There is no lossy intermediate step.

**The spec must be reconstruction-complete.** A developer (human or AI) reading only the
spec — without access to the original Blueprint or JSON — must be able to recreate the
Blueprint's behavior in either Blueprint or C++. The result doesn't need to be identical
to the original, but it must be functionally equivalent: same variables, same interfaces,
same logic behavior, same replication, same dependencies.

**Two paths to C++ must converge.** The PRIME DIRECTIVE pipeline supports two conversion paths:
- **Path A (direct):** Raw JSON IR → C++ converter tool → C++ code
- **Path B (via spec):** Raw JSON IR → AI-generated spec → C++ from spec (AI or human)

Both paths must arrive at functionally equivalent C++. If the spec loses information that
the direct converter would use, the spec is incomplete. This is the quality bar.

## How To Use This

When starting work on any UE project:

1. Run BlueprintSerializer to extract all Blueprints to JSON
2. Hand an AI agent the raw JSON exports + this Context Engineering System
3. The AI reads SCHEMA_REFERENCE.md to understand the data format
4. The AI reads ANALYSIS_PLAYBOOK.md for step-by-step guidance
5. The AI works through exports incrementally (one Blueprint at a time),
   committing specs and tracking progress via checklist
6. The AI produces per-Blueprint specs, system-level architecture docs,
   and project-wide knowledge documents

## Incremental Workflow

The AI does NOT need to process the entire corpus in one session. The expected workflow:

1. **Start a session** — load the playbook, check progress (git log / checklist)
2. **Pick the next Blueprint** — read its raw JSON export
3. **Produce the spec** — following the playbook's format and standards
4. **Commit the spec** — git commit with a clear message
5. **Update the checklist** — mark the Blueprint as done
6. **Repeat** — across as many sessions as needed until the corpus is complete

This means the context window constraint is per-Blueprint, not per-corpus. Most individual
Blueprint exports fit comfortably in a single AI context window.

---

## MANDATORY: Self-Improving Quality Loop

> **This section is a binding part of the PRIME DIRECTIVE.**
>
> A spec corpus that has not passed validation is NOT complete, even if every
> Blueprint has a corresponding spec file. The PRIME DIRECTIVE is only satisfied
> when specs pass the quality gates. An AI that generates specs and stops without
> validating them has not fulfilled the mandate.

### The Loop (execute after any spec generation run)

```
LOOP until corpus_thresholds_met():

  1. VALIDATE
     Run: python Plugins/BlueprintSerializer/Scripts/validate_specs.py
              docs_Lyra/BlueprintSpecs/
              Saved/BlueprintExports/BP_SLZR_All_*/
              --fix-list docs_Lyra/BlueprintSpecs/_VALIDATION_FAILURES.md

  2. READ FAILURES
     Read: docs_Lyra/BlueprintSpecs/_VALIDATION_FAILURES.md
     Check Summary: if Pass rate >= 95%, DONE.
     Otherwise: continue.

  3. IDENTIFY PATTERNS
     Count recurring error types. Any pattern appearing 5+ times:
       → Update ANALYSIS_PLAYBOOK.md with a new CRITICAL RULE.
       → Commit: "Playbook: add rule for [pattern]"

  4. REGENERATE
     Fix failing specs, ordered by error count (highest first).
     For each: read JSON export → read current spec → write corrected spec.
     Batch: 10-20 specs per commit. Commit after each batch.

  5. LOOP
     Go to step 1.

DONE when: Pass rate >= 95% (L1), >= 90% (L2), >= 85% (L3).
```

### Why This Is Part of the Mandate

A spec is only reconstruction-complete if it actually contains all the data.
The validator (`validate_specs.py`) mechanically checks that every IR field
(variable, function, CDO property, module dependency) is represented in the spec.
If fields are missing, the spec CANNOT be used to reconstruct the Blueprint.

The self-improvement rule: when the same type of failure appears in 5+ specs,
it means the AI's instructions (ANALYSIS_PLAYBOOK.md) had a gap. Fix the
instructions, then fix the specs. This improves every future session.

### Full Protocol

See `ITERATION_LOOP.md` in this directory for the complete step-by-step protocol
with examples, stopping criteria, edge case handling, and spec template.

### Quality Thresholds (from QUALITY_GATES.json)

| Layer | Check | Threshold |
|-------|-------|-----------|
| L1 | Structural completeness (every IR field in spec) | ≥ 95% |
| L2 | Value accuracy (types, flags, replication match IR) | ≥ 90% |
| L3 | Logic coverage (every graph entry point documented) | ≥ 85% |

---

## Scope Test

When deciding if a task is in scope for the PRIME DIRECTIVE:

> "Does this make it easier for an AI to build a game correctly on this UE project?"

If yes → in scope. If no → out of scope.

## Project Instances

Each project gets its own instance of the PRIME DIRECTIVE output:

- Extract for ProjectX → `Saved/BlueprintExports/`
- Specs for ProjectX → `Saved/BlueprintSpecs/`
- Knowledge base for ProjectX → project-specific output location

The **tools and methodology are universal**. The **output is project-specific**.

## Terms

| Term | Meaning |
|------|---------|
| PRIME DIRECTIVE | This mandate — the universal goal of AI-consumable UE project knowledge |
| BlueprintSerializer | The UE plugin that extracts Blueprint data to JSON IR |
| IR | Intermediate Representation — the structured JSON output |
| Context Engineering System | This set of documents that guide AI through spec generation |
| Spec | A document produced by AI that captures a Blueprint's complete technical detail |
| Corpus | The full set of Blueprints in a given UE project |

## Files In This System

| File | Purpose |
|------|---------|
| `PRIME_DIRECTIVE.md` | This file — the mandate, overview, and **self-improving loop** |
| `SCHEMA_REFERENCE.md` | Complete documentation of the JSON export schema |
| `ANALYSIS_PLAYBOOK.md` | Step-by-step spec generation guide with 8 CRITICAL RULES |
| `ITERATION_LOOP.md` | Autonomous quality loop — run to drive pass rate to 100% |
| `QUALITY_GATES.json` | Machine-readable quality thresholds and self-improvement rules |

**Reading order:** PRIME_DIRECTIVE → SCHEMA_REFERENCE → ANALYSIS_PLAYBOOK → start generating.
**After generating:** run the self-improving loop from ITERATION_LOOP.md until 100% passes.
