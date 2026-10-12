#!/usr/bin/env python3
"""Forensic gate: actual original DIAGNO before/after-SLOPE UG/VG vs P6D.

Compares the increment separately to distinguish upstream WIND1 differences
from SLOPE physical-law/temperature/terrain state differences. Source values
come from original CALMET temporary write-only instrumentation verified against
the untouched original MET.DAT hash. Never treats diagnostic success as parity.
"""
import argparse
import csv
import json
import math
import struct
from pathlib import Path

NX=NY=50
NZ=9
POINTS=((26,26),(1,26),(1,1),(13,13))
FIELDS=("u","v")
STAGES={7:"wind1",8:"wind1",9:"slope_corrected"}
LIMIT=1e-4

def read_f32(path):
    data=path.read_bytes()
    if len(data)!=NX*NY*NZ*4:raise RuntimeError(f"missing or bad-size P6D {path}")
    return struct.unpack("<22500f",data)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--original",required=True,type=Path)
    ap.add_argument("--product",required=True,type=Path)
    ap.add_argument("--out",required=True,type=Path)
    args=ap.parse_args()
    src={}
    with args.original.open(newline="") as f:
        for row in csv.DictReader(f):
            label=row["label"]
            kpt=int(row["kpt"]);i=int(row["i"]);j=int(row["j"]);k=int(row["k"])
            if kpt not in STAGES or (i,j) not in POINTS or k not in (1,2):
                raise RuntimeError("unexpected sampled original value")
            key=(label,kpt,i,j,k)
            if key in src:raise RuntimeError("duplicate original stage key")
            src[key]={"u":float(row["u"]),"v":float(row["v"]),
                      "qh":float(row["qh"]),"temp2d":float(row["temp2d"])}
    labels=sorted({x[0] for x in src})
    if len(labels)!=143 or len(src)!=143*3*4*2:
        raise RuntimeError("original source trace incomplete")
    result={"status":"P6D_D2_ORIGINAL_UPSTREAM_STAGE_LOCATED_NOT_PHYSICS_PASS",
            "original_source":"unmodified CALMET DIAGNO source-state hooks L5593/L5726/L5732",
            "note":"original/instrumented MET.DAT byte-equal; 143h, four cells k=1..2, U/V, QH/TEMP2D; original post-WIND1 separated from pre-SLOPE FRADJ",
            "frozen_wind_abs_limit_mps":LIMIT,"by_stage":{}, "worst_examples":[]}
    sums={}
    all_deltas={"pre":{},"post":{},"increment":{}}
    for label in labels:
        arrays={}
        for stage,prefix in STAGES.items():
            arrays[stage]={axis:read_f32(args.product/(f"{prefix}_{axis}_{label}.f32"))
                           for axis in FIELDS}
        for driver in ("qh","temp2d"):
            # Existing per-hour realcase producer exports the physical fields
            # it feeds SLOPE; a genuine original mismatch is an integration gap.
            blob=(args.product/f"{driver}_{label}.f32").read_bytes()
            if len(blob)!=NX*NY*4:raise RuntimeError("short "+driver+" "+label)
            arrays[driver]=struct.unpack("<2500f",blob)
        for i,j in POINTS:
            for k in (1,2):
                index=(k-1)*2500+(j-1)*NX+i-1
                for axis in FIELDS:
                    original_wind1=src[(label,7,i,j,k)][axis]
                    before=src[(label,8,i,j,k)][axis]
                    after=src[(label,9,i,j,k)][axis]
                    cpp_before=arrays[7][axis][index]
                    cpp_after=arrays[9][axis][index]
                    vals={"wind1":(original_wind1,cpp_before),
                          "fradj_original":(before-original_wind1,0.0),
                          "pre":(before,cpp_before),
                          "post":(after,cpp_after),
                          "increment":(after-before,cpp_after-cpp_before)}
                    for stage,(r,c) in vals.items():
                        key=(stage,axis,k)
                        e=c-r
                        entry=sums.setdefault(key,{"n":0,"exceedances":0,
                                                  "maximum_abs_error":0.,
                                                  "sum_sq":0.,"worst":None})
                        entry["n"]+=1
                        entry["sum_sq"]+=e*e
                        if abs(e)>LIMIT+1e-5*abs(r):entry["exceedances"]+=1
                        if abs(e)>=entry["maximum_abs_error"]:
                            entry["maximum_abs_error"]=abs(e)
                            entry["worst"]={"label":label,"i":i,"j":j,"level":k,
                                             "original":r,"cpp":c,"difference":e}
    # QH/TEMP2D are two-dimensional SLOPE drivers, not wind values.
    # Compare each XY-hour once (k==1 only), using a separate diagnostic gate.
    for label in labels:
        for i,j in POINTS:
            row=src[(label,8,i,j,1)]
            at=(j-1)*NX+i-1
            for driver in ("qh","temp2d"):
                # Read individual fields from the producer's actual hourly arrays.
                b=(args.product/f"{driver}_{label}.f32").read_bytes()
                if len(b)!=2500*4:raise RuntimeError("missing driver "+driver)
                c=struct.unpack_from("<f",b,4*at)[0]
                r=row[driver]
                e=c-r
                key=("driver",driver,1)
                stat=sums.setdefault(key,{"n":0,"exceedances":0,"maximum_abs_error":0.,
                                          "sum_sq":0.,"worst":None})
                stat["n"]+=1;stat["sum_sq"]+=e*e
                tol=0.001 if driver=="qh" else 0.001
                if abs(e)>tol+1e-5*abs(r):stat["exceedances"]+=1
                if abs(e)>=stat["maximum_abs_error"]:
                    stat["maximum_abs_error"]=abs(e)
                    stat["worst"]={"label":label,"i":i,"j":j,
                                   "original":r,"cpp":c,"difference":e}
    for (stage,axis,k),e in sums.items():
        result["by_stage"].setdefault(stage,{}).setdefault(axis,{})[f"k{k}"]={
            "n":e["n"],"exceedances":e["exceedances"],
            "maximum_abs_error":e["maximum_abs_error"],
            "rms_error":math.sqrt(e["sum_sq"]/e["n"]),
            "worst":e["worst"]}
    # Rank by first difference that must be understood to repair the full wind.
    diagnostic=[]
    for stage in ("wind1","fradj_original","pre","increment","post","driver"):
        n=sum(e["exceedances"] for (k,_,_),e in sums.items() if k==stage)
        maxerr=max(e["maximum_abs_error"] for (k,_,_),e in sums.items() if k==stage)
        diagnostic.append({"stage":stage,"exceedances":n,"max_abs":maxerr})
    result["stage_order"] = diagnostic
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2)+"\n")
    print("P6D_D2_PRE_POST_SLOPE_DIAGNOSTIC=COMPLETED_NOT_ACCEPTED")
    print(json.dumps(diagnostic,indent=2))
    for stage in ("pre","increment","post"):
        print(stage, json.dumps(result["by_stage"][stage]))


if __name__=="__main__":
    main()
