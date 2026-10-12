#!/usr/bin/env python3
"""True original WIND1 entry station-column comparison with C++ producer fixtures.

Only compares external station input values; does not claim WIND1 parity.
"""
import argparse
import csv
import json
import math
from pathlib import Path

def run():
    p=argparse.ArgumentParser()
    p.add_argument("--original",required=True,type=Path)
    p.add_argument("--fixtures",required=True,type=Path)
    p.add_argument("--index",required=True,type=Path)
    p.add_argument("--out",required=True,type=Path)
    a=p.parse_args()
    labels=a.index.read_text().split()
    if len(labels)!=143 or len(set(labels))!=143:
        raise RuntimeError("C++ producer must contain exactly 143 unique hours")
    original={}
    with a.original.open(newline="") as handle:
        for row in csv.DictReader(handle):
            key=tuple(int(row[x]) for x in ("hour","station","k"))
            if key in original:raise RuntimeError("duplicate original wind1 station input")
            original[key]=(float(row["u"]),float(row["v"]))
    if len(original)!=143*2*2:
        raise RuntimeError("original WIND1 station input coverage incomplete")
    mismatches=[]
    field_stats={k:{"compared":0,"max_abs":0.0,"exceedances":0}
                 for k in ("surface_k1","upper_k1_missing","upper_k2")}
    detailed=[]
    for h,label in enumerate(labels,1):
        file=a.fixtures/f"wind1_driver_{label}.inp"
        if not file.is_file():raise RuntimeError("missing WIND1 driver fixture "+str(file))
        lines=file.read_text().splitlines()
        if len(lines)!=11:
            raise RuntimeError(f"bad WIND1 fixture line count {label}: {len(lines)}")
        first=lines[0].split()
        if len(first)<6:raise RuntimeError("wind1 first line missing raw U/V")
        cpp_vals={
            "surface_k1":(float(first[4]),float(first[5])),
            "upper_k1_missing":tuple(map(float,lines[2].split())),
            "upper_k2":tuple(map(float,lines[3].split())),
        }
        indices={"surface_k1":(h,1,1),
                 "upper_k1_missing":(h,2,1),
                 "upper_k2":(h,2,2)}
        for name,index in indices.items():
            ref=original[index];cpp=cpp_vals[name]
            for axis,(x,y) in enumerate(zip(ref,cpp)):
                item=field_stats[name]
                item["compared"]+=1
                if name=="upper_k1_missing":
                    good=(x>=998 and y>=998)
                    err=0.0 if good else abs(x-y)
                else:
                    err=abs(x-y)
                    good=err<=2e-5+1e-5*abs(x)
                item["max_abs"]=max(item["max_abs"],err)
                if not good:
                    item["exceedances"]+=1
                    if len(mismatches)<40:
                        mismatches.append(dict(hour=h,label=label,field=name,axis="UV"[axis],
                                              original=x,cpp=y,abs_error=err))
                if h in (16,17,18,19,41,42,47,48,81,82):
                    detailed.append(dict(hour=h,label=label,field=name,axis="UV"[axis],
                                         original=x,cpp=y,abs_error=err))
    a.out.parent.mkdir(parents=True,exist_ok=True)
    report={"status":"WIND1_REAL_INPUT_COMPARISON_COMPLETED_NOT_PHYSICS_PASS",
            "n_hours":143,"fields":field_stats,"first_mismatches":mismatches,
            "focused_2022_hours":detailed,
            "note":"surface first level and upper sounding k1/k2 checked from actual original WIND1 entry; upper k1 compares missing-status because original/C++ sentinels can differ",
            "scientific_acceptance":"NOT_ACCEPTED"}
    a.out.write_text(json.dumps(report,indent=2)+"\n")
    print("P6D_WIND1_ENTRY_INPUT_DIAGNOSTIC=COMPLETED_NOT_ACCEPTED")
    print(json.dumps({"fields":field_stats,"first_mismatches":mismatches[:12]},indent=2))

if __name__=="__main__":run()
