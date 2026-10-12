#!/usr/bin/env python3
"""Forensic 3D bisection of true original DIAGNO/INTER2 before tail.

Compare six frozen hours × 50×50×9 cells × U/V; an exit-0 diagnosis is NOT
equivalent to P6D full-model parity.
"""
import argparse
import json
import math
import struct
from pathlib import Path

NX=NY=50
NZ=9
PLANE=NX*NY
TOTAL=PLANE*NZ
ABS_TOL=1e-4
REL_TOL=1e-5
LABELS=("2022-01-01_01","2022-01-01_02","2022-01-01_03",
        "2022-01-04_09","2022-01-04_10","2022-01-04_11")

def read(path):
    blob=path.read_bytes()
    if len(blob)!=4*TOTAL:raise RuntimeError(f"invalid 3D field shape {path}")
    values=struct.unpack("<22500f",blob)
    if any(not math.isfinite(v) for v in values):
        raise RuntimeError(f"nonfinite 3D field {path}")
    return values

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--original-dir",required=True,type=Path)
    ap.add_argument("--product",required=True,type=Path)
    ap.add_argument("--out",required=True,type=Path)
    a=ap.parse_args()
    fields={}
    examples=[]
    for axis in ("u","v"):
        summary={"max_abs_mps":0.0,"sum_sq":0.0,"exceedances":0,"n":0,
                 "per_level":[{"max_abs_mps":0.0,"exceedances":0,"n":0}
                              for _ in range(NZ)],"worst":None}
        for label in LABELS:
            original=read(a.original_dir/f"original_inter2_pretail_{axis}_{label}.f32")
            modern=read(a.product/f"inter2_{axis}_{label}.f32")
            for idx,(ref,cand) in enumerate(zip(original,modern)):
                err=abs(ref-cand)
                beyond=err>ABS_TOL+REL_TOL*abs(ref)
                lev=idx//PLANE
                x=(idx%PLANE)%NX+1
                y=(idx%PLANE)//NX+1
                summary["n"]+=1;summary["sum_sq"]+=err*err
                lv=summary["per_level"][lev];lv["n"]+=1
                lv["max_abs_mps"]=max(lv["max_abs_mps"],err)
                if beyond:
                    summary["exceedances"]+=1;lv["exceedances"]+=1
                    if len(examples)<24:
                        examples.append({"label":label,"axis":axis,"i":x,"j":y,
                                         "level":lev+1,"original":ref,
                                         "cpp":cand,"absolute_error_mps":err})
                if err>=summary["max_abs_mps"]:
                    summary["max_abs_mps"]=err
                    summary["worst"]={"label":label,"i":x,"j":y,"level":lev+1,
                                       "original":ref,"cpp":cand,"error":err}
        summary["rms_mps"]=math.sqrt(summary.pop("sum_sq")/summary["n"])
        fields[axis.upper()]=summary
    result={"status":"D2_REAL_ORIGINAL_PRETAIL_COMPARED_NOT_FULL_SCIENCE_PASS",
            "checkpoint":"original DIAGNO immediately after INTER2(U),INTER2(V), before SMOOTH/DIVCEL",
            "frozen_hour_labels":LABELS,"grid":[NX,NY,NZ],
            "tolerance_mps":{"absolute":ABS_TOL,"relative":REL_TOL},
            "source":"temporary write-only original Fortran instrumented run, frozen MET.DAT SHA guard",
            "candidates":"actual C++ pre-tail inter2_u/v_*.f32, not original-input synthetic fixture",
            "fields":fields,"first_exceedances":examples,
            "interpretation":"If this stage agrees but final U/V/W does not, investigate boundary/SMOOTH/DIVCEL. Otherwise investigate upstream first guess, station geometry/observations/weights and INTER2.",
            "scientific_verdict":"P6D_D2_NOT_PASS_UNTIL_ALL_143H_FULL_3D_UVW_PARITY"}
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+"\n")
    print("D2_PRETAIL_ORIGINAL_VS_CPP_DIAGNOSTIC=COMPLETED_NOT_FULL_PASS")
    print(json.dumps({key:{"max_abs_mps":v["max_abs_mps"],
                           "rms_mps":v["rms_mps"],
                           "exceedances":v["exceedances"],"n":v["n"],
                           "worst":v["worst"],"per_level":v["per_level"]}
                     for key,v in fields.items()},indent=2))

if __name__=="__main__":main()
