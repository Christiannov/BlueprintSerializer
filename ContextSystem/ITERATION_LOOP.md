# ITERATION LOOP — Autonomous Spec Quality Improvement Protocol

> **Give this document to any AI.** It contains everything needed to drive
> spec quality from the current state to 100% L1/L2/L3 pass rate autonomously,
> without human intervention between iterations.

---

## What You Are Doing

The PRIME DIRECTIVE (see `PRIME_DIRECTIVE.md`) requires that every Lyra Blueprint
spec be **reconstruction-complete** — readable alone to recreate the Blueprint.
485 specs were generated. A validator confirms whether they meet the bar.

Your job: **run the validator → read failures → fix root causes → loop until done.**

You do NOT need to understand the whole system. Each iteration is:
1. See what's broken
2. Fix it
3. Confirm improvement
4. Repeat

---

## Key File Paths

| File | Purpose |
|------|---------|
| `Plugins/BlueprintSerializer/Scripts/validate_specs.py` | Validation engine |
| `Plugins/BlueprintSerializer/ContextSystem/QUALITY_GATES.json` | Pass rate thresholds |
| `Plugins/BlueprintSerializer/ContextSystem/ANALYSIS_PLAYBOOK.md` | Spec generation rules |
| `Plugins/BlueprintSerializer/ContextSystem/PRIME_DIRECTIVE.md` | The mandate |
| `docs_Lyra/BlueprintSpecs/` | All spec files (*.md) |
| `docs_Lyra/BlueprintSpecs/_VALIDATION_FAILURES.md` | Current failures (auto-updated) |
| `docs_Lyra/BlueprintSpecs/_CHECKLIST.md` | Corpus checklist |
| `Saved/BlueprintExports/BP_SLZR_All_*/` | Source JSON IR for all BPs |

---

## Iteration Loop — Step by Step

### STEP 0: Orient (First Iteration Only)

```bash
# Check current branch and status
git log --oneline -5
git status

# Find the latest export directory
ls Saved/BlueprintExports/

# Run the validator to get baseline
python Plugins/BlueprintSerializer/Scripts/validate_specs.py \
  docs_Lyra/BlueprintSpecs/ \
  Saved/BlueprintExports/BP_SLZR_All_20260220_200043/ \
  --fix-list docs_Lyra/BlueprintSpecs/_VALIDATION_FAILURES.md
```

Record the starting pass rate (Summary section of `_VALIDATION_FAILURES.md`).

---

### STEP 1: Run the Validator

```bash
python Plugins/BlueprintSerializer/Scripts/validate_specs.py \
  docs_Lyra/BlueprintSpecs/ \
  Saved/BlueprintExports/BP_SLZR_All_20260220_200043/ \
  --fix-list docs_Lyra/BlueprintSpecs/_VALIDATION_FAILURES.md
```

This overwrites `_VALIDATION_FAILURES.md` with current results.

---

### STEP 2: Read and Analyze Failures

Read `docs_Lyra/BlueprintSpecs/_VALIDATION_FAILURES.md`.

Check the Summary section first:
- If **Pass rate ≥ 95%** → run one more validation to confirm, then **DONE**.
- Otherwise, continue.

Count recurring error patterns in the Failures table:

```python
# Quick pattern count (run as inline python or mentally):
# Count occurrences of each unique "Top Issue" message.
# Any pattern appearing in 5+ rows triggers a PLAYBOOK UPDATE (Step 5).
```

Categorize the failures into buckets (examples from known patterns):
- **Missing variables** — variable name in IR but not in spec table
- **Missing CDO section** — `classDefaultValueDelta` non-empty, no CDO section
- **Missing CDO properties** — CDO section exists but individual properties absent
- **Missing functions** — user function in IR but not in spec
- **Missing Logic section** — spec has no `## Logic` but Blueprint has nodes
- **Missing modules** — module in IR dependency closure but not in spec Modules list
- **Missing Purpose section** — no `## Purpose` section
- **Missing IR Notes** — no `## IR Notes` section

---

### STEP 3: Sort by Impact

Order failing BPs by error count (highest first). The Failures table in
`_VALIDATION_FAILURES.md` is already sorted this way.

Work through the list top-to-bottom. Each BP you fix is a win.

---

### STEP 4: Regenerate Failing Specs

For each failing BP (in order of error count):

#### 4a. Read the source JSON IR

```bash
# Find the JSON export for this Blueprint:
ls Saved/BlueprintExports/BP_SLZR_All_20260220_200043/ | grep <BlueprintName>
# Then read it:
# Read Saved/BlueprintExports/BP_SLZR_All_20260220_200043/<BlueprintName>.json
```

#### 4b. Read the current (failing) spec

```bash
# Read docs_Lyra/BlueprintSpecs/<BlueprintName>.md
```

#### 4c. Identify the gap

Compare the spec against the IR using the error messages from `_VALIDATION_FAILURES.md`.
Examples:
- "Variable 'X' in IR but missing from spec" → add row to Variables table
- "CDO has N overrides but spec has no CDO section" → add CDO Overrides section from `classDefaultValueDelta`
- "Function 'Y' in IR but missing from spec" → add row to Functions table

#### 4d. Write the corrected spec

Follow ALL rules in `ANALYSIS_PLAYBOOK.md` — especially the CRITICAL RULES at the top.

For EXHAUSTIVE VARIABLE LISTING (the most common failure):
- Read ALL entries in `detailedVariables[]` from the JSON
- Every entry (except `UberGraphFrame`) gets a row in the Variables table
- Do not skip, summarize, or abbreviate

For CDO SECTIONS:
- Read `classDefaultValueDelta` (not `classDefaultValues`)
- Every key in the delta gets a row in CDO Overrides
- Interpret complex values (curve data, struct literals) into human-readable form

For FUNCTIONS:
- Read ALL entries in `detailedFunctions[]`
- Skip: `ExecuteUbergraph`, `ExecuteUbergraph_*`, and `UserConstructionScript` if trivial
- Include: every other function, especially BT event overrides, custom functions, RPCs

For LOGIC:
- Only required if `totalNodeCount > 0` and EventGraph has nodes
- Follow execution chains from K2Node_Event → execution edges → pseudo-code
- See "Reading a Blueprint Export → 7. Structured Graphs" in ANALYSIS_PLAYBOOK.md

#### 4e. Write the spec

```
Write docs_Lyra/BlueprintSpecs/<BlueprintName>.md
```

#### 4f. Continue to next failing BP

Process as many as feasible in one session. Aim for batches of 10-20 BPs
before committing. Commit every 20 specs or at natural stopping points.

---

### STEP 5: Update the Playbook (If Pattern Threshold Hit)

If any error pattern appears **5 or more times** in `_VALIDATION_FAILURES.md`
and is NOT already covered by a CRITICAL RULE in `ANALYSIS_PLAYBOOK.md`:

1. Read `ANALYSIS_PLAYBOOK.md`
2. Add a new CRITICAL RULE covering that pattern
3. Explain: what triggers the error, how to avoid it, what to write instead
4. Commit the playbook update with message: `Playbook: add rule for [pattern]`

This is the **self-improvement mechanism**. Each iteration makes the playbook
smarter so future AI sessions start with better instructions.

---

### STEP 6: Commit Progress

```bash
git add docs_Lyra/BlueprintSpecs/
git commit -m "Spec: iteration N — fix M specs (pass rate X% → Y%)"
```

Include the pass rate change in the commit message so git history tracks progress.

---

### STEP 7: Re-run Validator

Go back to STEP 1.

Check the new pass rate. Compare to QUALITY_GATES.json thresholds:
- L1 pass rate ≥ 95% ✓
- L2 pass rate ≥ 90% ✓
- L3 pass rate ≥ 85% ✓

If all thresholds met: **DONE**. Commit the final validation report.
Otherwise: continue the loop.

---

## Stopping Criteria

The loop terminates when ALL of these are true:

1. `_VALIDATION_FAILURES.md` Summary shows:
   - **Pass rate ≥ 95%** (L1 threshold from `QUALITY_GATES.json`)
2. No L1 errors remain (only warnings are acceptable)
3. The validator exits with code 0

**100% L1 pass rate is the target.** Go for it.

---

## How to Handle Difficult Cases

### Blueprint has 20+ variables and many are missing

Don't spot-fix. **Regenerate the entire spec from scratch** using the JSON IR.
It's faster and more reliable than hunting for which variables are missing.
The JSON is authoritative. The spec should reflect it completely.

### Blueprint is data-only (no logic)

Many BPs exist only to set CDO values. For these:
- The CDO Overrides section IS the spec. Make it thorough.
- Interpret curve data, struct literals, and asset references.
- Skip empty sections (Variables, Functions, Components if truly empty).
- Still need: `## Identity`, `## Purpose`, `## CDO Overrides`, `## Dependencies`, `## IR Notes`

### Blueprint has complex graph logic

For BPs with 50+ nodes, work graph-by-graph:
1. Start with Identity, Variables, Functions (table rows first)
2. EventGraph: find all K2Node_Event nodes → one section each
3. Follow execution chains from each event → write pseudo-code
4. Function graphs: one section each, describe what they compute
5. Don't try to document every node — focus on WHAT the logic does, not the mechanism

### IR has unsupported nodes

If `compilerIRFallback.hasUnsupportedNodes` is true:
- Note which node types are unsupported in `## IR Notes`
- Describe what the overall logic LIKELY does based on surrounding context
- Mark gaps explicitly: "IR gap: [NodeType] nodes not exported — logic here is inferred"

---

## Current State (as of last validator run)

**Pass rate: 80.1% (386/482)**

Top remaining failure patterns (from `_VALIDATION_FAILURES.md`):
1. **Missing variables** — most high-error BPs are missing variables from their tables
2. **Missing CDO sections** — BTS_CheckAmmo, BTS_SetFocus, BTS_Shoot, etc.
3. **Missing CDO properties** — individual properties absent from existing CDO sections
4. **Missing functions** — user-authored functions not listed
5. **Missing Logic section** — BPs with nodes but no Logic section
6. **Missing modules** — dependency closure modules not in Dependencies list

Highest-priority BPs to fix (ordered by error count):
- B_Teleport (22 errors), GA_Weapon_Fire (22), B_TeamDeathMatchScoring (21)
- B_WeaponFire (18), B_FootStep (17), B_Pawn_Explorer (15), GA_AutoRespawn (15)
- B_WeaponImpacts (14), B_Bomb_Base (13), B_ControlPointScoring (13)
- B_ControlPointVolume (13), B_Grenade (12), GA_Hero_Dash (12)

---

## Self-Improvement Protocol

After each iteration, update this document's "Current State" section with:
- New pass rate
- New top failure patterns
- Updated priority list

After the playbook is updated (Step 5), verify the new rule would have caught
the pattern you just fixed. If not, refine the rule.

The goal: by iteration 3-5, ANALYSIS_PLAYBOOK.md should be comprehensive enough
that a fresh AI generating specs from scratch produces near-100% passing specs
WITHOUT needing the iteration loop at all.

---

## Validator Reference

```
validate_specs.py [spec_dir] [export_dir] [--fix-list path] [--report path]

Exit code 0: all specs pass
Exit code 1: one or more specs fail

Output: _VALIDATION_FAILURES.md with summary table + per-BP top error
JSON report: optional, contains per-BP per-field diagnostic detail
```

**Error severity:**
- `error` → causes FAIL, must fix for L1/L2/L3 compliance
- `warning` → causes "Pass with warnings", encouraged but not blocking

**Key L1 checks (errors):**
- Missing variable from detailedVariables[]
- Missing function from detailedFunctions[]
- CDO section absent when delta non-empty
- Missing Identity section (## Identity heading OR **Path:** + **Parent:** bold fields)
- IR has unsupported nodes but spec doesn't mention them

**Key L1 checks (warnings):**
- Individual CDO property missing from CDO table
- Module missing from Dependencies
- No Purpose section
- No IR Notes section
- Component section absent

**L3 checks (errors):**
- Blueprint has EventGraph nodes but spec has no Logic section

---

## Quick Spec Template

For rapid spec production, start from this template and fill in each section:

```markdown
# <BlueprintName>

## Identity
**Path:** `/Game/...`
**Parent:** `<ParentClass>` (`/Script/<Module>.<ParentClass>`)
**Generated Class:** `<BlueprintName>_C`
**Type:** <type>

## Purpose
<1-3 sentences: what it does, why it exists, what system owns it>

## CDO Overrides (vs Parent)
| Property | Value | Notes |
|----------|-------|-------|
| ... | ... | ... |

(Or: "Inherits all defaults from parent.")

## Variables
| Name | Type | Default | Replicated | Specifiers |
|------|------|---------|------------|------------|
| ... | ... | ... | No | ... |

## Functions
| Name | Access | Pure | Net | Params | Notes |
|------|--------|------|-----|--------|-------|
| ... | Public | No | None | ... | ... |

## Components
<tree or "None">

## Logic
<only if totalNodeCount > 0>

### Event: <EventName>
```
<EventName>(<params>)
  → <what happens>
  → if <condition>:
      → <then branch>
    else:
      → <else branch>
```

## Dependencies
**Modules:** <all from dependencyClosure.moduleNames>
**C++ Types:** <from classPaths, structPaths, enumPaths>
**Assets:** <from assetPaths>

## IR Notes
Full coverage — no unsupported nodes.
```

---

*This document is part of the PRIME DIRECTIVE Context Engineering System.*
*Update it after each iteration to reflect the current state of the corpus.*
