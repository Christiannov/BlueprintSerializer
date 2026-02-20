#!/usr/bin/env python3
"""analyze_b_weapon.py - B_Weapon JSON export C++ conversion completeness check."""
import json, sys
from pathlib import Path

JSON_PATH = Path(
    "C:/Users/jinph/Documents/Unreal Projects/LyraStarterGame"
    "/Saved/BlueprintExports/BP_SLZR_All_20260220_104347"
    "/BP_SLZR_Blueprint_B_Weapon_d2da07c8_20260220_104357.json"
)
SEP = "=" * 80
SUB = "-" * 60
gaps = []

def gap(msg):
    gaps.append(msg)
    print("  *** GAP: " + msg)

def section(title):
    print(""); print(SEP); print("  " + title); print(SEP)

def subsection(title):
    print(""); print(SUB); print("  " + title); print(SUB)

def exists_tag(val):
    if val is None: return "[MISSING]"
    if isinstance(val, (list, dict)) and len(val) == 0: return "[EMPTY]"
    return "[EXISTS]"

section("LOADING JSON")
print("Path: " + str(JSON_PATH))
if not JSON_PATH.exists(): print("ERROR: File not found"); sys.exit(1)
with JSON_PATH.open("r", encoding="utf-8") as fh: data = json.load(fh)
print("File loaded. Type: " + type(data).__name__)

section("TOP-LEVEL KEYS")
top_keys = list(data.keys()) if isinstance(data, dict) else []
print("Count: " + str(len(top_keys)))
for k in top_keys:
    val = data[k]; tag = exists_tag(val); t = type(val).__name__
    sz = ("  len=" + str(len(val))) if isinstance(val, (list, dict)) else ""
    print("  {:<45} {}  ({}{})".format(k, tag, t, sz))

section("1. CLASS DECLARATION")
bp_path     = data.get("blueprintPath") or data.get("generatedClassPath")
parent_path = data.get("parentClassPath")
class_spec  = data.get("classSpecifiers") or data.get("classFlags")
print(""); print("  blueprintPath / generatedClassPath : " + exists_tag(bp_path))
if bp_path: print("    " + str(bp_path))
else: gap("blueprintPath / generatedClassPath missing - cannot declare UCLASS name/path")
print(""); print("  parentClassPath : " + exists_tag(parent_path))
if parent_path: print("    " + str(parent_path))
else: gap("parentClassPath missing - cannot determine base class for UCLASS declaration")
print(""); print("  classSpecifiers / classFlags : " + exists_tag(class_spec))
if class_spec: print("    " + str(class_spec)[:200])
else: print("  (not present - will assume defaults: Blueprintable, BlueprintType)")

section("2. VARIABLES (detailedVariables) -> UPROPERTY")
detailed_vars = data.get("detailedVariables")
print("  detailedVariables : " + exists_tag(detailed_vars))
REQ_VAR = ["name","type","editAccess","replicationMode","category","tooltip"]
if detailed_vars is None:
    gap("detailedVariables missing - cannot generate UPROPERTY declarations")
elif len(detailed_vars) == 0: print("  (empty list)")
else:
    print("  Count: " + str(len(detailed_vars)))
    for i, var in enumerate(detailed_vars):
        vname = var.get("name", "<unnamed>")
        subsection("Variable [{}]: {}".format(i, vname))
        for field in REQ_VAR:
            val = var.get(field); tag = exists_tag(val)
            disp = str(val)[:100] if val is not None else ""
            print("    {:<25} {}  {}".format(field, tag, disp))
            if val is None and field in ("name","type"):
                gap("Variable [{}] missing field: {}".format(i, field))
        ds = var.get("declarationSpecifiers")
        print("    {:<25} {}  {}".format("declarationSpecifiers", exists_tag(ds), str(ds)[:100] if ds else ""))
        if i >= 9:
            rem = len(detailed_vars) - 10
            if rem > 0: print("  ... ({} more variables omitted)".format(rem))
            break

section("3. FUNCTIONS (detailedFunctions) -> UFUNCTION")
detailed_funcs = data.get("detailedFunctions")
print("  detailedFunctions : " + exists_tag(detailed_funcs))
REQ_FN = ["name","returnType","parameters","isOverride","callsParent","isStatic","isPure"]
if detailed_funcs is None:
    gap("detailedFunctions missing - cannot generate UFUNCTION declarations")
elif len(detailed_funcs) == 0: print("  (empty list)")
else:
    print("  Count: " + str(len(detailed_funcs)))
    for i, fn in enumerate(detailed_funcs):
        fname = fn.get("name","<unnamed>"); is_pub = fn.get("isPublic",fn.get("accessSpecifier","?"))
        ret = fn.get("returnType","<missing>"); ds = fn.get("declarationSpecifiers")
        print(""); print("  [{:3}] {}  access={}  return={}  spec={}".format(i,fname,is_pub,ret,str(ds)[:60]))
        for field in REQ_FN:
            val = fn.get(field)
            if val is None:
                print("      {:<22} [MISSING]".format(field))
                if field in ("name","returnType"):
                    gap("Function [{}] missing field: {}".format(fname, field))
        if i >= 14:
            rem = len(detailed_funcs) - 15
            if rem > 0: print(""); print("  ... ({} more functions omitted)".format(rem))
            break

section("4. COMPONENTS (detailedComponents) -> UPROPERTY")
detailed_comps = data.get("detailedComponents")
print("  detailedComponents : " + exists_tag(detailed_comps))
REQ_COMP = ["name","componentClass","parentOwnerClassName"]
if detailed_comps is None:
    gap("detailedComponents missing - cannot generate component UPROPERTY declarations")
elif len(detailed_comps) == 0: print("  (empty)")
else:
    print("  Count: " + str(len(detailed_comps)))
    for i, comp in enumerate(detailed_comps):
        cname = comp.get("name","<unnamed>"); cls = comp.get("componentClass","<missing>")
        owner = comp.get("parentOwnerClassName","<missing>")
        attach = comp.get("attachParent", comp.get("attachParentComponent","N/A"))
        assets = comp.get("assetReferences", comp.get("defaultValues", None))
        print(""); print("  [{}] {}".format(i, cname))
        print("    componentClass         : " + str(cls))
        print("    parentOwnerClassName   : " + str(owner))
        print("    attachParent           : " + str(attach))
        if assets: print("    assetRefs/defaults     : " + str(assets)[:200])
        for field in REQ_COMP:
            if comp.get(field) is None:
                gap("Component [{}] missing field: {}".format(cname, field))
        if i >= 9:
            rem = len(detailed_comps) - 10
            if rem > 0: print("  ... ({} more components omitted)".format(rem))
            break

section("5. DEPENDENCY CLOSURE (dependencyClosure) -> #include")
dep_closure = data.get("dependencyClosure")
print("  dependencyClosure : " + exists_tag(dep_closure))
if dep_closure is None:
    gap("dependencyClosure missing - cannot determine #include directives")
else:
    classes_list = dep_closure.get("classes")
    native_hints = dep_closure.get("nativeIncludeHints")
    uds_schemas  = dep_closure.get("userDefinedStructSchemas")
    ude_schemas  = dep_closure.get("userDefinedEnumSchemas")
    cnt = len(classes_list) if isinstance(classes_list, list) else "N/A"
    print(""); print("  classes              : {}  count={}".format(exists_tag(classes_list), cnt))
    if isinstance(classes_list, list):
        for c in classes_list[:12]: print("    " + str(c))
        if len(classes_list) > 12: print("    ... ({} total)".format(len(classes_list)))
    print(""); print("  nativeIncludeHints   : " + exists_tag(native_hints))
    if native_hints is None:
        gap("nativeIncludeHints missing - cannot map classes to C++ headers for #include")
    elif isinstance(native_hints, dict):
        print("  count=" + str(len(native_hints)))
        for cls_name, header in list(native_hints.items())[:25]:
            print("    {:<50} -> {}".format(cls_name, header))
        if len(native_hints) > 25: print("    ... ({} total entries)".format(len(native_hints)))
    elif isinstance(native_hints, list):
        print("  count=" + str(len(native_hints)))
        for item in native_hints[:25]: print("    " + str(item))
    else: print("  value: " + str(native_hints))
    cnt2 = len(uds_schemas) if isinstance(uds_schemas, list) else "N/A"
    print(""); print("  userDefinedStructSchemas : {}  count={}".format(exists_tag(uds_schemas), cnt2))
    if uds_schemas:
        show = uds_schemas[:5] if isinstance(uds_schemas, list) else [str(uds_schemas)]
        for s in show: print("    " + str(s)[:120])
    cnt3 = len(ude_schemas) if isinstance(ude_schemas, list) else "N/A"
    print(""); print("  userDefinedEnumSchemas   : {}  count={}".format(exists_tag(ude_schemas), cnt3))
    if ude_schemas:
        show = ude_schemas[:5] if isinstance(ude_schemas, list) else [str(ude_schemas)]
        for e in show: print("    " + str(e)[:120])

section("6. DELEGATE SIGNATURES (delegateSignatures)")
delegate_sigs = data.get("delegateSignatures")
print("  delegateSignatures : " + exists_tag(delegate_sigs))
if delegate_sigs is None: print("  (field absent - no custom delegates or not captured)")
elif len(delegate_sigs) == 0: print("  (empty)")
else:
    print("  Count: " + str(len(delegate_sigs)))
    for i, d in enumerate(delegate_sigs):
        dname  = d.get("name") or d.get("signatureName", "<unnamed>")
        params = d.get("parameters", [])
        print("  [{}] {}  params({}): {}".format(i, dname, len(params), params))

section("7. TIMELINES (timelines) -> FTimeline members")
timelines = data.get("timelines")
print("  timelines : " + exists_tag(timelines))
if timelines is None: print("  (field absent)")
elif len(timelines) == 0: print("  (empty)")
else:
    print("  Count: " + str(len(timelines)))
    for i, tl in enumerate(timelines):
        tname = tl.get("name") or tl.get("timelineName","<unnamed>")
        update_fn = tl.get("updateFunctionName") or tl.get("updateFunction","?")
        tracks = tl.get("tracks", []); key_count = sum(len(t.get("keys",[])) for t in tracks if isinstance(t,dict))
        print("  [{}] {}  updateFn={}  tracks={}  totalKeys={}".format(i,tname,update_fn,len(tracks),key_count))
        for t in tracks[:5]:
            tn = t.get("name") if isinstance(t,dict) else str(t)
            tk = len(t.get("keys",[])) if isinstance(t,dict) else "?"
            print("       track: {}  keys={}".format(tn, tk))

section("8. STRUCTURED GRAPHS (structuredGraphs) -> function bodies")
struct_graphs = data.get("structuredGraphs")
print("  structuredGraphs : " + exists_tag(struct_graphs))
if struct_graphs is None:
    gap("structuredGraphs missing - cannot generate function body implementations")
elif len(struct_graphs) == 0: print("  (empty)")
else:
    print("  Count: " + str(len(struct_graphs)))
    for i, g in enumerate(struct_graphs):
        gname = g.get("graphName") or g.get("name","<unnamed>")
        nodes = g.get("nodes", [])
        print("  [{:3}] {:<55}  nodes={}".format(i, gname, len(nodes)))
    event_graph = None
    for g in struct_graphs:
        gn = (g.get("graphName") or g.get("name","")).lower()
        if "eventgraph" in gn or "event graph" in gn: event_graph = g; break
    if event_graph is None and struct_graphs: event_graph = struct_graphs[0]
    if event_graph:
        gname = event_graph.get("graphName") or event_graph.get("name","")
        nodes = event_graph.get("nodes", [])
        subsection("First 5 nodes from: " + gname)
        for i, node in enumerate(nodes[:5]):
            ntype = node.get("nodeType") or node.get("type","<unknown>")
            meta  = {k: v for k, v in node.items() if k not in ("nodeType","type","pins","children","nodes")}
            print("  Node [{}] type={}".format(i, ntype))
            for mk, mv in list(meta.items())[:8]:
                print("    {}: {}".format(mk, str(mv)[:100]))

section("9. CDO DEFAULTS (cdoProperties / classDefaultObject)")
cdo = data.get("cdoProperties") or data.get("classDefaultObject")
if "cdoProperties" in data: cdo_key = "cdoProperties"
elif "classDefaultObject" in data: cdo_key = "classDefaultObject"
else: cdo_key = None
print("  {} : {}".format(cdo_key or "cdoProperties / classDefaultObject", exists_tag(cdo)))
if cdo is None:
    gap("cdoProperties / classDefaultObject missing - cannot initialize default property values")
elif isinstance(cdo, dict):
    print("  Count: " + str(len(cdo)))
    for k, v in list(cdo.items())[:15]:
        print("    {:<45} : {}".format(k, str(v)[:100]))
    if len(cdo) > 15: print("    ... ({} total)".format(len(cdo)))
elif isinstance(cdo, list):
    print("  Count: " + str(len(cdo)))
    for item in cdo[:10]: print("    " + str(item)[:120])

section("CONSOLIDATED GAP REPORT")
if not gaps:
    print(""); print("  No gaps detected. All required fields present.")
else:
    print(""); print("  {} gap(s) found that would block or impair C++ conversion:".format(len(gaps))); print("")
    for i, g in enumerate(gaps, 1): print("  [{:2}] {}".format(i, g))

print(""); print(SEP); print("  Analysis complete."); print(SEP)
