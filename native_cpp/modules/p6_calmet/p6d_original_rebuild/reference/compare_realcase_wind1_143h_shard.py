#!/usr/bin/env python3
"""Four-way sharded 143-hour ORIGINAL WIND1 source vs independent C++ test.

Original CALMET's WIND1+neutral SIMILT is extracted unmodified from its
Git-SHA-pinned calmet.for. All selected 50x50x9 U/V first guesses from the
real 2022 Hefei case are checked against BOTH (a) independently source-
ported C++ and (b) the real-case producer's original FLOAT32 first guess.

IMPORTANT: this is original WIND1 on the *same P6D-supplied station/sounding
fixture*; does not establish that the fixture equals original PREPDI station
profiles. Does not imply full DIAGNO or 143h final wind numerical parity.
"""
from __future__ import annotations
import argparse
import datetime as dt
import json
import math
import sys
from pathlib import Path

REFERENCE=Path(__file__).resolve().parents[2]/"reference"
sys.path.insert(0,str(REFERENCE))
from compare_original_wind1_firstguess import (
    DRIVER, TOLERANCE, NX, NY, NZ, compare, extract, run
)
from compare_original_wind1_realcase_first_hour import (
    execute, compare_float32_field
)

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--vendor",required=True,type=Path)
    p.add_argument("--case",required=True,type=Path)
    p.add_argument("--p6d",required=True,type=Path)
    p.add_argument("--cpp",required=True,type=Path)
    p.add_argument("--out",required=True,type=Path)
    p.add_argument("--first-hour",required=True,type=int)
    p.add_argument("--last-hour",required=True,type=int)
    p.add_argument("--fortran",default="gfortran-11")
    a=p.parse_args()
    if not(1<=a.first_hour<=a.last_hour<=143):
        raise SystemExit("source WIND1 143h shard must be inside 1..143")
    a.out.mkdir(parents=True,exist_ok=True)
    fixture=a.out/"fixtures";fields=a.out/"p6d_fields"
    fixture.mkdir(exist_ok=True)
    fields.mkdir(exist_ok=True)
    # Sequentially generated station and first-guess fields retain
    # the original previous-hour state progression until the shard end.
    execute(a.p6d,"--case",a.case,"--hours",a.last_hour,
            "--fixtures",fixture,"--out",fields,timeout=1000)
    originals=extract(a.vendor,a.out)
    driver=a.out/"wind1_unchanged_source_driver.for"
    driver.write_text(DRIVER,encoding="utf-8")
    original=a.out/"original_wind1"
    execute(a.fortran,"-std=legacy","-O0","-fno-automatic",
            "-ffixed-line-length-none","-fallow-argument-mismatch",
            "-I"+str(a.vendor),*originals,driver,"-o",original)
    results=[]
    failure_count=0
    first_errors=[]
    max_selected=max_emitted=0.
    for hour_id in range(a.first_hour,a.last_hour+1):
        label=(dt.datetime(2022,1,1)+dt.timedelta(hours=hour_id)).strftime("%Y-%m-%d_%H")
        inp=fixture/f"wind1_driver_{label}.inp"
        if not inp.is_file():
            raise RuntimeError(f"Missing frozen hourly station fixture {label}")
        ref=a.out/"hour.original.txt"
        chosen=a.out/"hour.independent.txt"
        run(original,inp,ref)
        run(a.cpp,inp,chosen)
        selected=compare(ref,chosen,label)
        emitted=compare_float32_field(ref,fields,label)
        if selected["status"]!="PASS" or emitted["status"]!="PASS":
            failure_count+=1
            if len(first_errors)<8:
                first_errors.append({
                    "hour_index":hour_id,"hour":label,
                    "independent":selected["first_errors"][:4],
                    "realcase_emitted":emitted["first_errors"][:4],
                })
        max_selected=max(max_selected,selected["max_abs_error_mps"])
        max_emitted=max(max_emitted,emitted["max_abs_error_mps"])
        results.append({
            "hour_index":hour_id,"hour":label,
            "source_vs_independent":selected["status"],
            "source_vs_realcase_emitted":emitted["status"],
            "max_abs_independent_mps":selected["max_abs_error_mps"],
            "max_abs_realcase_mps":emitted["max_abs_error_mps"],
        })
    report={
        "status":"PASS_ORIGINAL_WIND1_REALCASE_SHARD" if not failure_count else "FAIL",
        "source":"unmodified Git-pinned CALMET 6.5.0 WIND1+neutral SIMILT",
        "scope":("original first-guess with P6D-provided real-case PREPDI "
                 "station fixtures, NOT independent original PREPDI/full DIAGNO"),
        "shard":[a.first_hour,a.last_hour],
        "hours_compared":len(results),
        "components_compared":len(results)*NX*NY*NZ*2,
        "original_real4_tolerance_mps":TOLERANCE,
        "max_source_vs_independent_mps":max_selected,
        "max_source_vs_realcase_emitted_mps":max_emitted,
        "hours_failed":failure_count,
        "first_errors":first_errors,
        "hours":results,
    }
    report_path=a.out/"wind1_143h_shard_parity.json"
    report_path.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({k:report[k] for k in (
        "status","shard","hours_compared","components_compared",
        "max_source_vs_independent_mps","max_source_vs_realcase_emitted_mps",
        "hours_failed","first_errors")},indent=2))
    if failure_count:raise SystemExit(1)


if __name__=="__main__":
    main()
