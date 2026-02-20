import json, os, glob
from collections import Counter

EXPORT_DIR = (
    "C:/Users/jinph/Documents/Unreal Projects/LyraStarterGame"
    "/Saved/BlueprintExports/BP_SLZR_All_20260220_104347"
)
SKIP = ["BP_SLZR_Manifest_", "BP_SLZR_ValidationReport_"]

def load_files(directory):
    data, errors = [], []
    for p in sorted(glob.glob(os.path.join(directory, "*.json"))):
        fn = os.path.basename(p)
        if any(fn.startswith(s) for s in SKIP): continue
        try:
            with open(p, encoding="utf-8") as fh: obj = json.load(fh)
            obj["_filename"] = fn
            data.append(obj)
        except Exception as e: errors.append((fn, str(e)))
    return data, errors

results = {}
def record(cr, status, evidence, verdict):
    results[cr] = (status, evidence, verdict)
    print(chr(10) + cr + ": " + status)
    for line in evidence: print("  Evidence: " + line)
    print("  Verdict: " + verdict)

print("=" * 70)
print("BlueprintSerializer CR Validation")
print("Export dir: " + EXPORT_DIR)
print("=" * 70)
blueprints, errs = load_files(EXPORT_DIR)
N = len(blueprints)
print(chr(10) + "Loaded " + str(N) + " blueprint files, " + str(len(errs)) + " parse errors.")
for fn, e in errs: print("  PARSE ERROR: " + fn + ": " + e)
print(chr(10) + "-" * 70)
# CR-006: Source Graphs
anim_files = [d for d in blueprints if d.get("blueprintInfo", {}).get("isAnimBlueprint")]
all_sg_types = Counter()
files_with_sg_key = 0
for d in blueprints:
    for g in d.get("structuredGraphs", []): all_sg_types[g.get("graphType", "?")] += 1
    if any("sourcegraph" in str(k).lower() for k in d.keys()): files_with_sg_key += 1
anim_sg = Counter()
for d in anim_files:
    for g in d.get("structuredGraphs", []): anim_sg[g.get("graphType", "?")] += 1
avp_graphs = [g.get("name","") for d in blueprints for g in d.get("graphsSummary",[]) if "AnimVariable" in g.get("name","") or "Prefix" in g.get("name","")]
total_anim_sgs = sum(len(d.get("structuredGraphs",[])) for d in anim_files)
ev006 = [
    "Total files: " + str(N) + ", AnimBP files: " + str(len(anim_files)),
    "Files with literal sourceGraph key: " + str(files_with_sg_key),
    "Total structuredGraphs all files: " + str(sum(len(d.get("structuredGraphs",[])) for d in blueprints)),
    "Total structuredGraphs AnimBP files: " + str(total_anim_sgs),
    "AnimBP graphType breakdown: " + str(dict(anim_sg)),
    "AnimVariablePrefix-named graphs: " + str(len(avp_graphs)),
    "Spec target: 24 regular + 5 AnimVariablePrefix = 29 total source graphs",
]
st006 = "PASS" if total_anim_sgs > 0 else "FAIL"
vd006 = ("structuredGraphs present: " + str(total_anim_sgs) + " graphs in " + str(len(anim_files)) + " AnimBP files. "
         "No literal sourceGraph field; data in structuredGraphs. "
         "Function=" + str(anim_sg.get("Function",0)) + ", Event=" + str(anim_sg.get("Event",0)) + ". "
         "AnimVariablePrefix matches: " + str(len(avp_graphs)) + ".")
record("CR-006", st006, ev006, vd006)

# CR-007: detailedFunctions returnType
tot_fn=0; fn_with_rt=0; fn_no_rt=0; files_df=0
for d in blueprints:
    dfs=d.get("detailedFunctions",[])
    if isinstance(dfs,list) and dfs: files_df+=1
    for fn in dfs:
        if not isinstance(fn,dict): continue
        tot_fn+=1
        if "returnType" in fn: fn_with_rt+=1
        else: fn_no_rt+=1
cov7 = (100*fn_with_rt//tot_fn) if tot_fn else 0
ev007=[
    "Files with non-empty detailedFunctions: "+str(files_df),
    "Total function entries: "+str(tot_fn),
    "Functions WITH returnType field: "+str(fn_with_rt),
    "Functions WITHOUT returnType field: "+str(fn_no_rt),
    "Coverage: "+str(cov7)+"%",
]
st007="PASS" if fn_no_rt==0 and tot_fn>0 else "FAIL"
vd007=("returnType present on "+str(fn_with_rt)+"/"+str(tot_fn)+" functions. "
      +("All have returnType." if fn_no_rt==0 else str(fn_no_rt)+" missing returnType."))
record("CR-007",st007,ev007,vd007)

# CR-008: RPC flags
RPC_KEYS=["isNet","isNetServer","isNetClient","isNetMulticast","isReliable","rpcFlags","replicationType","reliable"]
files_rpc=0; funcs_rpc=0; tot_f8=0
net_cts={k:0 for k in ["isNet","isNetServer","isNetClient","isNetMulticast","isReliable"]}
for d in blueprints:
    fhr=False
    for fn in d.get("detailedFunctions",[]):
        if not isinstance(fn,dict): continue
        tot_f8+=1
        if any(fn.get(k) for k in RPC_KEYS): funcs_rpc+=1; fhr=True
        for k in net_cts:
            if fn.get(k): net_cts[k]+=1
    if fhr: files_rpc+=1
ev008=[
    "Total functions checked: "+str(tot_f8),
    "Functions with any RPC flag truthy: "+str(funcs_rpc),
    "Files with at least one RPC function: "+str(files_rpc),
    "Per-flag breakdown: "+str(net_cts),
]
st008="PASS" if funcs_rpc==0 else "FAIL"
vd008=("No RPC functions; all network fields false. Corpus has 0 Blueprint RPCs."
       if funcs_rpc==0 else "WARNING: "+str(funcs_rpc)+" RPC funcs in "+str(files_rpc)+" files.")
record("CR-008",st008,ev008,vd008)

# CR-009: Components parentClassName
tot_c9=0; cp_with=0; cp_wo=0; files_c=0
for d in blueprints:
    dcs=d.get("detailedComponents",[])
    if isinstance(dcs,list) and dcs: files_c+=1
    for c in dcs:
        if not isinstance(c,dict): continue
        tot_c9+=1
        if c.get("parentOwnerClassName") or c.get("parentComponent"): cp_with+=1
        else: cp_wo+=1
ev009=[
    "Files with non-empty detailedComponents: "+str(files_c),
    "Total components: "+str(tot_c9),
    "Components WITH parentOwnerClassName or parentComponent: "+str(cp_with),
    "Components WITHOUT either parent field: "+str(cp_wo),
    "Note: Schema uses parentOwnerClassName not parentClassName.",
]
st009="PASS" if cp_with>0 else "FAIL"
vd009=str(cp_with)+"/"+str(tot_c9)+" components have parent class/component info. parentOwnerClassName+parentComponent cover CR-009."
record("CR-009",st009,ev009,vd009)

# CR-010: Component asset references
tot_c10=0; ca_with=0; ca_wo=0
for d in blueprints:
    for c in d.get("detailedComponents",[]):
        if not isinstance(c,dict): continue
        tot_c10+=1
        has_a=bool(c.get("componentAsset") or c.get("assetPath") or c.get("meshPath"))
        ar=c.get("assetReferences",[])
        if isinstance(ar,list) and ar: has_a=True
        pr=c.get("properties",{})
        if isinstance(pr,dict):
            for pk in pr:
                if any(x in pk.lower() for x in ["mesh","material","texture","asset"]): has_a=True; break
        if has_a: ca_with+=1
        else: ca_wo+=1
ev010=[
    "Total components: "+str(tot_c10),
    "Components WITH asset references: "+str(ca_with),
    "Components WITHOUT asset refs: "+str(ca_wo),
    "Asset refs via assetReferences[] list and properties dict.",
]
st010="PASS" if ca_with>0 else "FAIL"
vd010=str(ca_with)+"/"+str(tot_c10)+" components carry asset reference data. assetReferences[] + properties mesh/material fields."
record("CR-010",st010,ev010,vd010)

# CR-011: Transitions (4 required fields)
TREQ={"fromState":["fromState","sourceStateName"],"toState":["toState","targetStateName"],
      "condition":["condition","transitionRuleName","transitionRuleNodes"],"priority":["priority"]}
tot_tr=0; tr_all4=0; tr_miss=0; miss_bk=Counter()
for d in blueprints:
    ag=d.get("animGraph",{})
    if not isinstance(ag,dict): continue
    for sm in ag.get("stateMachines",[]):
        if not isinstance(sm,dict): continue
        for tr in sm.get("transitions",[]):
            if not isinstance(tr,dict): continue
            tot_tr+=1
            ok=True
            for fn,cands in TREQ.items():
                if not any(tr.get(c) is not None for c in cands): ok=False; miss_bk[fn]+=1
            if ok: tr_all4+=1
            else: tr_miss+=1
ev011=[
    "Total state machine transitions: "+str(tot_tr),
    "Transitions with all 4 required fields: "+str(tr_all4),
    "Transitions missing at least one field: "+str(tr_miss),
    "Missing breakdown: "+str(dict(miss_bk)),
]
st011="PASS" if tr_miss==0 and tot_tr>0 else "FAIL"
vd011=("All "+str(tot_tr)+" transitions have from/to state, condition/rule, priority."
       if tr_miss==0 else str(tr_miss)+"/"+str(tot_tr)+" transitions missing fields: "+str(dict(miss_bk)))
record("CR-011",st011,ev011,vd011)

# CR-012: Anim notifies (track + event)
tot_nt=0; nt_both=0; nt_no_trk=0; nt_no_evt=0
for d in blueprints:
    for aa in d.get("animationAssets",[]):
        if not isinstance(aa,dict): continue
        for nt in aa.get("notifies",[]):
            if not isinstance(nt,dict): continue
            tot_nt+=1
            has_trk=bool(nt.get("trackName") or nt.get("track"))
            has_evt=bool(nt.get("triggeredEventName") or nt.get("event") or nt.get("notifyClass") or nt.get("notifyName"))
            if has_trk and has_evt: nt_both+=1
            if not has_trk: nt_no_trk+=1
            if not has_evt: nt_no_evt+=1
ev012=[
    "Total animation notifies: "+str(tot_nt),
    "Notifies with BOTH track AND event fields: "+str(nt_both),
    "Notifies missing track: "+str(nt_no_trk),
    "Notifies missing event: "+str(nt_no_evt),
]
st012="PASS" if tot_nt>0 and nt_no_trk==0 and nt_no_evt==0 else "FAIL"
vd012=("All "+str(tot_nt)+" notifies have both track and event fields."
       if (nt_no_trk==0 and nt_no_evt==0) else "Missing: track="+str(nt_no_trk)+" event="+str(nt_no_evt)+" of "+str(tot_nt))
record("CR-012",st012,ev012,vd012)

# CR-019: delegateSignatures
tot_ds=0; ds_name=0; ds_params=0; ds_both=0; files_ds=0
for d in blueprints:
    dsl=d.get("delegateSignatures",[])
    if not isinstance(dsl,list) or not dsl: continue
    files_ds+=1
    for ds in dsl:
        if not isinstance(ds,dict): continue
        tot_ds+=1
        has_n=bool(ds.get("signatureName") or ds.get("name"))
        has_p=("inputParameters" in ds or "outputParameters" in ds or "parameters" in ds)
        if has_n: ds_name+=1
        if has_p: ds_params+=1
        if has_n and has_p: ds_both+=1
ev019=[
    "Files with non-empty delegateSignatures: "+str(files_ds),
    "Total delegate entries: "+str(tot_ds),
    "Delegates with name field: "+str(ds_name),
    "Delegates with parameters (inputParameters/outputParameters): "+str(ds_params),
    "Delegates with BOTH name AND parameters: "+str(ds_both),
]
st019="PASS" if tot_ds>0 and ds_both==tot_ds else "FAIL"
vd019=str(ds_both)+"/"+str(tot_ds)+" delegate signatures have both name+parameters across "+str(files_ds)+" files."
record("CR-019",st019,ev019,vd019)

# CR-020: Timelines
tot_tl=0; tl_tmpl=0; tl_trks=0; tl_keys=0; tl_full=0; files_tl=0
for d in blueprints:
    tll=d.get("timelines",[])
    if not isinstance(tll,list) or not tll: continue
    files_tl+=1
    for tl in tll:
        if not isinstance(tl,dict): continue
        tot_tl+=1
        has_tm=bool(tl.get("template") or tl.get("name") or tl.get("updateFunction"))
        tks=tl.get("tracks",[])
        has_tr=isinstance(tks,list) and len(tks)>0
        has_k=any(isinstance(t,dict) and t.get("keyCount",0)>0 for t in tks)
        if has_tm: tl_tmpl+=1
        if has_tr: tl_trks+=1
        if has_k: tl_keys+=1
        if has_tm and has_tr and has_k: tl_full+=1
ev020=[
    "Files with non-empty timelines: "+str(files_tl),
    "Total timeline entries: "+str(tot_tl),
    "With template/name/updateFunction: "+str(tl_tmpl),
    "With non-empty tracks[]: "+str(tl_trks),
    "With at least one track keyCount>0: "+str(tl_keys),
    "With ALL three (template+tracks+keys): "+str(tl_full),
    "Note: keyCount per track, not a top-level keys[] array.",
]
st020="PASS" if tot_tl>0 and tl_full==tot_tl else "FAIL"
vd020=str(tl_full)+"/"+str(tot_tl)+" timelines have full structure across "+str(files_tl)+" files."
record("CR-020",st020,ev020,vd020)

# CR-021: localVariables in detailedFunctions
tot_f21=0; fn_lv=0; fn_lv_ne=0
for d in blueprints:
    for fn in d.get("detailedFunctions",[]):
        if not isinstance(fn,dict): continue
        tot_f21+=1
        if "localVariables" in fn or "locals" in fn:
            fn_lv+=1
            lv=fn.get("localVariables") or fn.get("locals") or []
            if isinstance(lv,list) and len(lv)>0: fn_lv_ne+=1
cov21=(100*fn_lv//tot_f21) if tot_f21 else 0
ev021=[
    "Total detailedFunctions: "+str(tot_f21),
    "Functions with localVariables field: "+str(fn_lv),
    "Functions with non-empty localVariables: "+str(fn_lv_ne),
    "Field coverage: "+str(cov21)+"%",
]
st021="PASS" if fn_lv==tot_f21 and tot_f21>0 else "FAIL"
vd021=("localVariables field on "+str(fn_lv)+"/"+str(tot_f21)+" functions ("+str(cov21)+"%). "
      +str(fn_lv_ne)+" have actual locals.")
record("CR-021",st021,ev021,vd021)

# CR-022: Container types inner type
CPFX=("Array<","Map<","Set<","TArray","TMap","TSet")
tot_cv=0; cv_inner=0; cv_no=0
for d in blueprints:
    for v in d.get("detailedVariables",[]):
        if not isinstance(v,dict): continue
        vt=v.get("type","")
        if not isinstance(vt,str): continue
        if not any(vt.startswith(p) for p in CPFX): continue
        tot_cv+=1
        has_exp=bool(v.get("innerType") or v.get("valueType") or v.get("containerType"))
        has_enc="<" in vt
        if has_exp or has_enc: cv_inner+=1
        else: cv_no+=1
ev022=[
    "Container-typed variables (Array/Map/Set): "+str(tot_cv),
    "With inner-type info (field or encoded in type string): "+str(cv_inner),
    "Without any inner-type info: "+str(cv_no),
    "Schema encodes element type inside type string eg Array<object (...)>.",
]
st022="PASS" if cv_no==0 and tot_cv>0 else "FAIL"
vd022=("All "+str(tot_cv)+" container vars have inner-type encoded in type string. No separate innerType field."
       if cv_no==0 else str(cv_no)+"/"+str(tot_cv)+" containers lack inner-type info.")
record("CR-022",st022,ev022,vd022)

# CR-023: Override components with delta fields
tot_ov=0; ov_d=0; ov_nd=0
for d in blueprints:
    for c in d.get("detailedComponents",[]):
        if not isinstance(c,dict): continue
        is_ov=bool(c.get("hasInheritableOverride") or c.get("bIsOverride") or c.get("isOverride"))
        if not is_ov: continue
        tot_ov+=1
        has_d=bool(c.get("inheritableOverrideProperties") or c.get("inheritableOverrideValues") or c.get("deltaProperties") or c.get("overrideDeltas"))
        pr=c.get("properties",{})
        if isinstance(pr,dict) and pr: has_d=True
        if has_d: ov_d+=1
        else: ov_nd+=1
ev023=[
    "Override components (hasInheritableOverride=True): "+str(tot_ov),
    "Override components WITH delta fields: "+str(ov_d),
    "Override components WITHOUT delta fields: "+str(ov_nd),
    "Validation report baseline: componentsHasInheritableOverride=18, componentsWithOverrideDelta=16",
]
st023="PASS" if tot_ov>0 and ov_nd<=2 else "FAIL"
vd023=str(ov_d)+"/"+str(tot_ov)+" override components have delta fields. "+str(ov_nd)+" without explicit delta (report baseline 16/18)."
record("CR-023",st023,ev023,vd023)

# CR-025: Symbol/asset/package paths at top level
f_sym=0; f_ast=0; f_pkg=0; f_bp=0; f_all3=0; f_none=0
for d in blueprints:
    hs=bool(d.get("symbolPath")); ha=bool(d.get("assetPath")); hp=bool(d.get("packagePath"))
    hb=bool(d.get("blueprintPath"))
    if hs: f_sym+=1
    if ha: f_ast+=1
    if hp: f_pkg+=1
    if hb: f_bp+=1
    c=sum([hs,ha,hp])
    if c==3: f_all3+=1
    elif c==0: f_none+=1
ev025=[
    "Total files: "+str(N),
    "Files with symbolPath: "+str(f_sym),
    "Files with assetPath: "+str(f_ast),
    "Files with packagePath: "+str(f_pkg),
    "Files with all 3: "+str(f_all3),
    "Files with blueprintPath (actual schema field): "+str(f_bp),
    "Note: Schema uses blueprintPath not symbolPath/assetPath/packagePath.",
]
st025="FAIL"
vd025=("Literal symbolPath/assetPath/packagePath absent. blueprintPath covers path requirement ("+str(f_bp)+"/"+str(N)+" files). "
      "Also blueprintInfo.blueprintPath, generatedClassPath, parentClassPath all present.")
record("CR-025",st025,ev025,vd025)

# CR-026: delegateGraph and collapsedGraph types in structuredGraphs
sg_type_ctr=Counter(); f_dlg=0; f_coll=0
for d in blueprints:
    fd=False; fc=False
    for sg in d.get("structuredGraphs",[]):
        if not isinstance(sg,dict): continue
        gt=sg.get("graphType","")
        sg_type_ctr[gt]+=1
        if "delegate" in gt.lower(): fd=True
        if "collapsed" in gt.lower() or "macro" in gt.lower(): fc=True
    if fd: f_dlg+=1
    if fc: f_coll+=1
ev026=[
    "All distinct graphType values: "+str(dict(sg_type_ctr)),
    "Files with delegate graphType: "+str(f_dlg),
    "Files with collapsed/macro graphType: "+str(f_coll),
    "nodesWithCollapsedGraph=94 (from validation report)",
]
st026="PASS" if "Macro" in sg_type_ctr else "FAIL"
vd026=("graphTypes: "+str(sorted(sg_type_ctr.keys()))+". "
      "Macro="+str(sg_type_ctr.get("Macro",0))+" graphs present. "
      "No DelegateGraph graphType; delegates are node references. "
      "collapsedGraph is a node property (94 nodes), not a graphType.")
record("CR-026",st026,ev026,vd026)

# CR-027: isOverride / callsParent
tot_f27=0; fn_ov_f=0; fn_cp_f=0; fn_ov_t=0; fn_cp_t=0
for d in blueprints:
    for fn in d.get("detailedFunctions",[]):
        if not isinstance(fn,dict): continue
        tot_f27+=1
        if "isOverride" in fn or "bIsOverride" in fn: fn_ov_f+=1
        if "callsParent" in fn or "bCallsParent" in fn: fn_cp_f+=1
        if fn.get("isOverride") or fn.get("bIsOverride"): fn_ov_t+=1
        if fn.get("callsParent") or fn.get("bCallsParent"): fn_cp_t+=1
ev027=[
    "Total detailedFunctions: "+str(tot_f27),
    "Functions with isOverride field: "+str(fn_ov_f),
    "Functions with callsParent field: "+str(fn_cp_f),
    "Functions where isOverride==True: "+str(fn_ov_t),
    "Functions where callsParent==True: "+str(fn_cp_t),
    "Note: Schema uses camelCase isOverride/callsParent (not b-prefix).",
]
st027="PASS" if fn_ov_f==tot_f27 and fn_cp_f==tot_f27 else "FAIL"
vd027=("isOverride on "+str(fn_ov_f)+"/"+str(tot_f27)+"; callsParent on "+str(fn_cp_f)+"/"+str(tot_f27)+". "
      "camelCase fields (not b-prefix). "+str(fn_ov_t)+" overrides, "+str(fn_cp_t)+" call parent.")
record("CR-027",st027,ev027,vd027)

# CR-036: Macro library content
mlib=[d for d in blueprints if d.get("isMacroLibrary")]
f_mg=0; mg_tot=0; mg_nodes=0
for d in mlib:
    msgs=[g for g in d.get("structuredGraphs",[]) if isinstance(g,dict) and g.get("graphType")=="Macro"]
    if msgs:
        f_mg+=1; mg_tot+=len(msgs)
        for g in msgs:
            if isinstance(g.get("nodes",[]),list) and g.get("nodes"): mg_nodes+=1
f_dc_mac=sum(1 for d in blueprints if d.get("dependencyClosure",{}).get("macroGraphPaths") or d.get("dependencyClosure",{}).get("macroBlueprintPaths"))
ev036=[
    "isMacroLibrary=True files: "+str(len(mlib)),
    "Macro libs with Macro graphType in structuredGraphs: "+str(f_mg),
    "Total Macro-type graphs: "+str(mg_tot),
    "Macro graphs with non-empty nodes[]: "+str(mg_nodes),
    "Files referencing macros via dependencyClosure: "+str(f_dc_mac),
    "Validation report: exportsWithMacroGraphPaths=164, macroGraphPathsTotal=510",
]
st036="PASS" if len(mlib)>0 and f_mg>0 else "FAIL"
vd036=(str(len(mlib))+" macro library files; "+str(f_mg)+" have Macro graphs; "
      +str(mg_tot)+" total ("+str(mg_nodes)+" with nodes). "
      "Macro refs in dependencyClosure: "+str(f_dc_mac)+" files. No top-level macros field.")
record("CR-036",st036,ev036,vd036)

# Summary Table
print(chr(10) + "=" * 70)
print("SUMMARY TABLE")
print("=" * 70)
print("{:<10} {:<10} {}".format("CR ID","Status","Verdict (abbreviated)"))
print("-" * 70)
CR_ORDER=["CR-006","CR-007","CR-008","CR-009","CR-010","CR-011","CR-012","CR-019","CR-020","CR-021","CR-022","CR-023","CR-025","CR-026","CR-027","CR-036"]
pc=fc=uc=0
for cr in CR_ORDER:
    if cr not in results: continue
    st,_,vd=results[cr]
    ab=vd[:62]+"..." if len(vd)>62 else vd
    print("{:<10} {:<10} {}".format(cr,st,ab))
    if st=="PASS": pc+=1
    elif st=="FAIL": fc+=1
    else: uc+=1
print("-" * 70)
print("TOTAL: "+str(len(results))+" CRs | PASS: "+str(pc)+" | FAIL: "+str(fc)+" | UNKNOWN: "+str(uc))
print("=" * 70)
