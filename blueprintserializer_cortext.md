# BlueprintSerializer — Universal Core Context

> **This file is the single source of truth for any AI agent (Claude, ChatGPT, Gemini, Copilot, or any other) working on the BlueprintSerializer plugin.**
> All agent config files (`AGENTS.md`, `CLAUDE.md`, `GEMINI.md`, etc.) point here.
> Read this entire file before touching any code or running any build command.

---

## What This Plugin Is

BlueprintSerializer is an Unreal Engine 5 editor plugin that serializes Blueprint assets to structured JSON for AI-assisted C++ conversion workflows. Its primary mandate is **C++ conversion fidelity** — producing JSON that contains every piece of information a converter needs to generate correct, idiomatic C++ from a Blueprint class.

**Plugin location:** `Plugins/BlueprintSerializer/`
**Canonical scope docs:** `FULL_EXTRACTION_DEFINITION.md`, `CONVERTER_READY_EXTRACTION_SPEC.md`
**Live work tracker:** `CONVERTER_READY_TODO.md`
**Regression history:** `REGRESSION_LOG.md`
**Regression baseline:** `REGRESSION_BASELINE.json`

---

## 🔴 CRITICAL: How to Build This Plugin — Read Before Every Build

### Plugins Have No Standalone Build Target. Always Build the Host Project.

A UE plugin's `.uplugin` file declares modules but defines **no build target**.
UBT requires a `.Target.cs` file to know the root of the build graph.
That file lives in the **host game project**, not in the plugin.

```
✅ CORRECT:
Build.bat <ProjectName>Editor Win64 Development "<ProjectName>.uproject"

❌ ALWAYS FAILS — plugins have no .Target.cs:
Build.bat BlueprintSerializerEditor Win64 Development ...
Build.bat BlueprintSerializer Win64 Development ...
```

The plugin DLL (`UnrealEditor-BlueprintSerializer.dll`) is produced as a
side-effect of building the host editor target. It is never built on its own.

### How to Derive the Target Name for Any Host Project

1. Look in `<ProjectRoot>/Source/` for any file matching `*.Target.cs`
2. The filename without `.Target.cs` is the base target name
3. Append `Editor` for editor builds

**Examples:**
| `.Target.cs` file found | Correct build target |
|---|---|
| `Source/LyraEditor.Target.cs` | `LyraEditor` |
| `Source/MyGameEditor.Target.cs` | `MyGameEditor` |
| `Source/ShooterEditor.Target.cs` | `ShooterEditor` |

### Full Windows Command Pattern

```bash
powershell -Command "& 'C:\\Program Files\\Epic Games\\UE_5.X\\Engine\\Build\\BatchFiles\\Build.bat' \
  <ProjectName>Editor Win64 Development \
  '\"<AbsolutePath>\\<ProjectName>.uproject\"' \
  -waitmutex 2>&1"
```

For the current Lyra project specifically:
```bash
powershell -Command "& 'C:\\Program Files\\Epic Games\\UE_5.6\\Engine\\Build\\BatchFiles\\Build.bat' \
  LyraEditor Win64 Development \
  '\"C:\\Users\\jinph\\Documents\\Unreal Projects\\LyraStarterGame\\LyraStarterGame.uproject\"' \
  -waitmutex 2>&1"
```

**This rule applies in every project this plugin is ever used in. Do not skip it.**

---

## Regression Workflow (Required After Every Non-Trivial Change)

```
1. Build the host project editor target (see above)
2. Run the regression suite:
   powershell -ExecutionPolicy Bypass -File Plugins/BlueprintSerializer/Scripts/Run-RegressionSuite.ps1
3. Wait for suitePass=True in the output
4. Verify artifact files exist:
   Saved/BlueprintExports/BP_SLZR_RegressionRun_<timestamp>.json      ← suitePass field
   Saved/BlueprintExports/BP_SLZR_All_<timestamp>/BP_SLZR_ValidationReport_<timestamp>.json
   Saved/BlueprintExports/BP_SLZR_AnimCurveAudit_<timestamp>.json
5. Update REGRESSION_BASELINE.json if new metrics were added
6. Append an entry to REGRESSION_LOG.md
7. Commit plugin + superproject
```

**Never claim success from shell exit code alone.** Always verify `suitePass=True` in the JSON artifact.

See `REGRESSION_HARNESS.md` for full harness documentation.

---

## Key Source Files

| File | Role |
|---|---|
| `Source/BlueprintSerializer/Private/BlueprintAnalyzer.cpp` | Core extraction logic — `AnalyzeNodeToStruct`, `IsNodeTypeKnownSupported`, all K2Node handlers |
| `Source/BlueprintSerializer/Public/BlueprintAnalyzer.h` | Data structs: `FBS_NodeData`, `FBS_FunctionInfo`, `FBS_ParamInfo`, `FBS_LocalVarInfo`, etc. |
| `Source/BlueprintSerializer/Private/BlueprintExtractorCommands.cpp` | Console commands, regression suite orchestration, validator metric counters |
| `Source/BlueprintSerializer/BlueprintSerializer.Build.cs` | Module dependencies — add new external module headers here |
| `Scripts/Run-RegressionSuite.ps1` | Host-side regression harness (launches UE, waits, closes, reports) |
| `REGRESSION_BASELINE.json` | Minimum metric thresholds — gates that must pass every run |
| `REGRESSION_LOG.md` | Chronological run history with metrics and lessons |
| `CONVERTER_READY_TODO.md` | Live backlog of remaining C++ conversion fidelity work |

---

## Key Architectural Patterns

### IsNodeTypeKnownSupported()
Static `TSet<FName>` in `BlueprintAnalyzer.cpp`. Classifies a node type as "supported" so it does not fall through to the unsupported-node fallback path. Add new node type names here when adding a handler.

### AnalyzeNodeToStruct()
Per-node enrichment function. Uses `Cast<UK2Node_Foo>(K2)` blocks to populate `meta.*` fields on the node's `nodeProperties` JSON object. This is where all K2Node-specific metadata lives.

### Accessing Protected/Private UE Members
Many K2Node UPROPERTYs are declared `protected:` or in private headers. The established workaround is UE reflection:
```cpp
// For a protected FName UPROPERTY named "SomeField":
if (FNameProperty* P = FindFProperty<FNameProperty>(Node->GetClass(), TEXT("SomeField")))
{
    FName Val = P->GetPropertyValue_InContainer(Node);
    if (!Val.IsNone()) AddMeta(TEXT("meta.someField"), Val.ToString());
}

// For a protected TSubclassOf<> / FClassProperty:
if (FClassProperty* P = FindFProperty<FClassProperty>(Node->GetClass(), TEXT("SomeClass")))
{
    UClass* Val = Cast<UClass>(P->GetObjectPropertyValue_InContainer(Node));
    if (Val) AddMeta(TEXT("meta.someClass"), Val->GetPathName());
}
```

### FMemberReference — GetMemberParentPackage() Returns UPackage*, Not FName
```cpp
// ✅ CORRECT:
if (UPackage* Pkg = Ref.GetMemberParentPackage())
    AddMeta(TEXT("meta.ownerPackage"), Pkg->GetName());

// ❌ WRONG — pointer does not have .IsNone() or .ToString():
if (!Ref.GetMemberParentPackage().IsNone()) ...
```

### Adding a New Module Dependency
Edit `BlueprintSerializer.Build.cs` → `PrivateDependencyModuleNames`. Then rebuild. Common modules needed for node type headers:
- `InputCore` — FKey methods (GetFName, GetDisplayName)
- `GameplayAbilitiesEditor` — K2Node_LatentAbilityCall
- `GameplayTasksEditor` — K2Node_LatentGameplayTaskCall
- `InputBlueprintNodes` — K2Node_EnhancedInputAction
- `EnhancedInput` — UInputAction full definition

---

## Regression Baseline (Current)

485 Blueprint files, 0 parse errors, 14 validation gates (all must pass).

Key corpus counts as of 2026-02-20:
- `nodesWithTunnel`: 1949
- `nodesWithPromotableOp`: 1000
- `nodesWithBranch`: 1054, `nodesWithReroute`: 2949
- `nodesWithSetPersistentFrame`: 341
- `nodesWithCallMaterialParamCollection`: 209
- `nodesWithGetArrayItem`: 209, `nodesWithAddComponent`: 199
- `nodesWithPropertyAccess`: 85, `nodesWithBaseAsyncTask`: 73
- `nodesWithGetSubsystem`: 72, `nodesWithMakeArray`: 71
- `variablesTotal`: 3133, `functionsTotal`: 936, `componentsTotal`: 818

See `REGRESSION_BASELINE.json` for the full and authoritative list.

---

## Work Priority

1. **C++ conversion fidelity** — extraction completeness, node meta enrichment, dependency closure
2. **Regression gate coverage** — new gates for newly emitted fields
3. **Animation/editor-heavy work** — human-led; automation assists but does not lead

Persistent mandate: keep `CONVERTER_READY_TODO.md` in normalized backlog mode so new AI sessions inherit priority framing without rereading the full log.

---

## Status Language Definitions

| Term | Meaning |
|---|---|
| `done` | Implemented AND validated with corpus evidence (`suitePass=True`) |
| `in_progress` | Implemented but validation not yet complete |
| `blocked` | Validation blocked by corpus or tooling constraints (not a code failure) |

---

*Last updated: 2026-02-20. Update this file whenever architectural patterns, module deps, or build procedures change.*
