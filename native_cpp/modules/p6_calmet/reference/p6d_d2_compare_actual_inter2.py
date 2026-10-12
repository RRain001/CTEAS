#!/usr/bin/env python3
"""Compare true DIAGNO/INTER2 in-process operand traces (not fixtures).

A successful exit means inputs compared and discrepancies documented. Numerical
equality is assessed separately and *never* inferred from diagnostic exit 0.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path

POINTS={(26,26),(1,26),(1,1),(13,13)}
INTEGER=("call","i","j","k","station","selected_rank","nk","nintrp")
FIELDS=("station_x_km","station_y_km","observed_mps","rs_km",
        "wo","step1_mps","weighted_sum_before","weight_sum_before",
        "r_km","step_weight","selected_rank","nk","nintrp")
# Audit thresholds only; frozen scientific wind-field tolerance is unchanged.
LIMITS={k:(0 if k in INTEGER else 1.0e-5) for k in FIELDS}
LIMITS["observed_mps"]=2.e-5
LIMITS["step1_mps"]=2.e-5
LIMITS["station_x_km"]=2.e-5
LIMITS["station_y_km"]=2.e-5
LIMITS["rs_km"]=2.e-5
LIMITS["step_weight"]=2.e-5
LIMITS["weighted_sum_before"]=2.e-4
LIMITS["weight_sum_before"]=2.e-4


def read(path: Path):
    with path.open(newline="") as f:
        reader=csv.DictReader(f)
        expect=("call","i","j","k","station","selected_rank","rs_km",
                "station_x_km","station_y_km","observed_mps","wo","step1_mps",
                "weighted_sum_before","weight_sum_before","r_km","step_weight","nk","nintrp")
        if tuple(reader.fieldnames or ())!=expect:
            raise ValueError(f"{path}: unexpected trace columns {reader.fieldnames}")
        d={}
        for row in reader:
            r={}
            for k,value in row.items():
                r[k]=int(value) if k in INTEGER else float(value.replace("D","E"))
                if k not in INTEGER and not math.isfinite(r[k]):
                    raise ValueError(f"nonfinite {k} in {path}")
            key=tuple(r[k] for k in ("call","i","j","k","station"))
            if key in d:raise ValueError(f"duplicate {key} in {path}")
            d[key]=r
    expected=143*2*len(POINTS)*2*2
    if len(d)!=expected:raise ValueError(f"{path}: {len(d)} != {expected}")
    return d


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--original",required=True,type=Path)
    ap.add_argument("--cpp",required=True,type=Path)
    ap.add_argument("--out",required=True,type=Path)
    a=ap.parse_args()
    original=read(a.original)
    cpp=read(a.cpp)
    if original.keys()!=cpp.keys():
        raise ValueError(f"keys differ, example missing original={list(cpp.keys()-original.keys())[:5]}, "
                         f"missing cpp={list(original.keys()-cpp.keys())[:5]}")
    field_stats={}
    for f in FIELDS:
        max_abs=-1.
        worst_key=None
        mismatch=0
        for key in original:
            v=original[key][f]
            x=cpp[key][f]
            err=abs(v-x)
            if err>max_abs:
                max_abs=err;worst_key=key
            tolerance=LIMITS[f]+1.e-5*abs(v) if f not in INTEGER else 0
            if err>tolerance:mismatch+=1
        field_stats[f]={
            "maximum_absolute_difference":max_abs,
            "mismatches":mismatch,
            "n":len(original),
            "first_worst_record": {"call":worst_key[0],"i":worst_key[1],
                "j":worst_key[2],"k":worst_key[3],"station":worst_key[4],
                "original":original[worst_key][f],"cpp":cpp[worst_key][f]},
            "diagnostic_abs_tolerance":LIMITS[f],
        }
    categories=("station_x_km","station_y_km","observed_mps","rs_km",
                "selected_rank","nk","nintrp","wo","step1_mps","weighted_sum_before",
                "weight_sum_before","r_km","step_weight")
    first_per_stage=[]
    for k in categories:
        if field_stats[k]["mismatches"]:
            first_per_stage.append(k)
    focus=[]
    for call in (1,2,3,4,161,162,163,164):
        for point in ((26,26),(1,26),(1,1),(13,13)):
            for lev in (1,2):
                for station in (1,2):
                    key=(call,point[0],point[1],lev,station)
                    r=original[key];c=cpp[key]
                    focus.append({"key":key,"original":{f:r[f] for f in categories},
                                  "cpp":{f:c[f] for f in categories}})
    result={
        "status":"D2_ACTUAL_INTER2_OPERANDS_COMPARED_NOT_ACCEPTANCE",
        "definition":"Original full CALMET 143h real DIAGNO INTER2 invocations versus C++ in-kernel operands; no synthetic fixture or original wind fed as candidate",
        "calls":286,"grid_cells":list(map(list,POINTS)),"levels":[1,2],
        "samples":len(original),"fields":field_stats,
        "discrepant_fields_in_processing_order":first_per_stage,
        "focused_first_and_hour81_82_comparisons":focus,
        "scientific_verdict":"P6D_D2_NOT_PASS_UNTIL_FROZEN_143H_UVW_EQUALS_REFERENCE",
    }
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+"\n")
    print("D2_ACTUAL_INTER2_TRACE_COMPARISON=COMPLETED_NOT_SCIENTIFIC_PASS")
    print(json.dumps({"samples":len(original),
                      "discrepant_fields_in_processing_order":first_per_stage,
                      "fields":{k:{"max_abs":v["maximum_absolute_difference"],
                                   "mismatches":v["mismatches"],
                                   "worst":v["first_worst_record"]}
                                for k,v in field_stats.items()}},indent=2))


if __name__=="__main__":
    main()
