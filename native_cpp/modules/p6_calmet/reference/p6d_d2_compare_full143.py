#!/usr/bin/env python3
"""Strict original CALMET vs native P6D full-143h three-dimensional U/V/W gate.

This is an *oracle-input-scoped* numerical comparison, NOT proof of standalone
P6D physics. Real original snapshots taken at DIAGNO L6062 and L6218 are
from a temporary source-instrumented binary whose MET.DAT digest exactly
matches the immutable untouched original; native requires original GAMMA,
ZII and EL, which remain non-native inputs.
"""
import argparse
import datetime as dt
import json
import math
import struct
from pathlib import Path

NX,NY,NZ=50,50,9
NCELL=NX*NY
N3=NCELL*NZ
NF=NCELL*(NZ+1)
NH=143
ABS_TOL=1e-4
REL_TOL=1e-5
STAGES=(
    ("inter2", "original_inter2_pretail_143h.f32",
     (("U","inter2_u",N3),("V","inter2_v",N3))),
    ("post_diag", "original_final143_uvw.f32",
     (("U","blend_u",N3),("V","blend_v",N3),("W","w_face",NF))),
)

def floats(bytestring,count,name):
    if len(bytestring)!=count*4:
        raise RuntimeError(f"{name} expected {count*4} bytes got {len(bytestring)}")
    values=struct.unpack("<"+str(count)+"f",bytestring)
    if not all(math.isfinite(x) and abs(x)<9999 for x in values):
        raise RuntimeError(f"{name} has nonfinite/invalid original or candidate wind")
    return values

def make_summary(nlevels):
    return {"compared_values":0,"exceedances":0,"maximum_abs_error_mps":0.0,
            "rms_sum_squared":0.0,"worst":None,
            "per_level":[{"level":i+1,"compared_values":0,
                          "exceedances":0,"maximum_abs_error_mps":0.0}
                         for i in range(nlevels)],
            "per_hour_exceedances":{}}

def check(ref,candidate,s,label,stage,axis):
    if len(ref)!=len(candidate):raise RuntimeError("field length mismatch")
    hourly=0
    for idx,(x,y) in enumerate(zip(ref,candidate)):
        delta=y-x
        err=abs(delta)
        lev=idx//NCELL
        layer=s["per_level"][lev]
        s["compared_values"]+=1
        s["rms_sum_squared"]+=delta*delta
        layer["compared_values"]+=1
        if err>s["maximum_abs_error_mps"]:
            s["maximum_abs_error_mps"]=err
            s["worst"]={"hour":label,"level":lev+1,
                        "i":idx%NX+1,"j":idx%NCELL//NX+1,
                        "original_mps":x,"native_mps":y,"error_mps":err,
                        "stage":stage,"component":axis}
        if err>layer["maximum_abs_error_mps"]:
            layer["maximum_abs_error_mps"]=err
        if err>ABS_TOL+REL_TOL*abs(x):
            s["exceedances"]+=1
            layer["exceedances"]+=1
            hourly+=1
    if hourly:s["per_hour_exceedances"][label]=hourly

def main():
    a=argparse.ArgumentParser()
    a.add_argument("--original-dir",required=True,type=Path)
    a.add_argument("--product",required=True,type=Path)
    a.add_argument("--out",required=True,type=Path)
    a.add_argument("--require-zero-exceedances",action="store_true")
    args=a.parse_args()
    # Hourly end label is the original hourly counter, 2022-01-01_01
    # through 2022-01-06_23 inclusive; never infer from mtime.
    labels=[(dt.datetime(2022,1,1)+dt.timedelta(hours=h)).strftime("%Y-%m-%d_%H")
            for h in range(1,NH+1)]
    idx=args.product/"produced_hours.txt"
    if not idx.is_file() or idx.read_text().split()!=labels:
        raise RuntimeError("native hour-index labels do not match 143 original hours")
    results={}
    for stage,filename,fields in STAGES:
        original_file=args.original_dir/filename
        stride=sum(n*4 for _,_,n in fields)
        if not original_file.is_file() or original_file.stat().st_size!=NH*stride:
            raise RuntimeError(f"{stage} original float32 stream not 143h x expected geometry")
        stats={axis:make_summary(n//NCELL) for axis,_,n in fields}
        with original_file.open("rb") as f:
            for label in labels:
                for axis,prefix,n in fields:
                    original=floats(f.read(n*4),n,f"original {stage}/{axis}/{label}")
                    candidate_file=args.product/(prefix+"_"+label+".f32")
                    if not candidate_file.is_file():
                        raise RuntimeError(f"missing original-comparable native stage {candidate_file}")
                    values=floats(candidate_file.read_bytes(),n,str(candidate_file))
                    check(original,values,stats[axis],label,stage,axis)
            if f.read(1):raise RuntimeError(f"{stage}: unexpected trailing original bytes")
        for axis,_,n in fields:
            sm=stats[axis]
            if sm["compared_values"]!=NH*n:raise RuntimeError("incomplete 143h comparison")
            sm["rms_error_mps"]=math.sqrt(sm.pop("rms_sum_squared")/sm["compared_values"])
        results[stage]=stats
    exceeds=sum(s["exceedances"] for fields in results.values() for s in fields.values())
    output={"status":"SOURCE_AUTHENTIC_ORIGINAL_FULL143_UVW_ORACLE_SCOPED",
            "original_source":"unchanged original CALMET v6.5.0 instrumented write-only, byte-identical frozen MET.DAT; six original DGM kpt10 source snaps independently byte-equal",
            "input_qualification":"candidate driven by original EL/ZII/GAMMA oracles; not full native P6D operational acceptance",
            "hours":NH,"first_hour":labels[0],"last_hour":labels[-1],
            "shape":{"nx":NX,"ny":NY,"nz":NZ,"w_faces":NZ+1},
            "threshold":{"abs_mps":ABS_TOL,"rel":REL_TOL},
            "stages":results,"total_exceedances":exceeds,
            "scientific_verdict":("FULL143_3D_NUMERIC_PARITY_WITH_ORACLES_PASS"
                                  if exceeds==0 else "FULL143_3D_NUMERIC_PARITY_NOT_PASS"),
            "production_p6d_verdict":"NOT_PASS_UNTIL_EL_ZII_GAMMA_NATIVE_AND_UNORACLED"}
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(output,indent=2,sort_keys=True)+"\n")
    print("P6D_D2_FULL143_3D_ORIGINAL_COMPARISON="+output["scientific_verdict"])
    print(json.dumps({"total_exceedances":exceeds,
          "stages":{k:{f:{"n":v["compared_values"],"exceedances":v["exceedances"],
                          "max_abs_mps":v["maximum_abs_error_mps"],
                          "rms_mps":v["rms_error_mps"],"worst":v["worst"]}
                        for f,v in fields.items()}
                    for k,fields in results.items()}},indent=2))
    if args.require_zero_exceedances and exceeds:
        raise SystemExit("FULL143_3D_NUMERIC_PARITY_NOT_PASS; original reference unchanged")

if __name__=="__main__":main()
