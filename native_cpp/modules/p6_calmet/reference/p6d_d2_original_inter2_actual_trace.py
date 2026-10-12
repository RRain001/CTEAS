#!/usr/bin/env python3
"""Trace selected actual original CALMET INTER2 calls without modifying vendor files.

Uses the frozen Hefei 2022 case, compiles original and a temporary write-only
instrumented copy, and asserts the resulting MET.DAT SHA-256 is identical.
The output is from *real* DIAGNO INTER2 invocations, not a fixture replay.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import original_internal_state_dump as dgm

BASE_SHA = dgm.ORIGINAL_CALMET_SHA256
MET_SHA = dgm.TARGET_MET_DAT_SHA256
SELECTED = ((26, 26), (1, 26), (1, 1), (13, 13))
# Zero other physics instructions altered. Fixed vendor source line anchors.
ANCHORS = {
    8243: "      double precision wt2,wt3",
    8247: "      DATA npts/4/",
    8568: "      wt3 = 1./(R**2)",
}
BEFORE = {8243: "      DOUBLE PRECISION WT"}
AFTER = {8247: "C", 8568: "c"}

DECL = """
c --- P6D D-2 diagnostic only, no original arithmetic changed
      integer d2call,d2rank,d2ll,d2mm
      save d2call
      data d2call/0/
"""
START = """
c --- P6D D-2 source-constrained hourly U/V call counter
      d2call=d2call+1
      if(d2call.eq.1)then
         open(91,file='p6d_inter2_actual.txt',
     1        form='formatted',status='replace')
      endif
      if(nst.ne.2)stop 'D2 expected exactly two wind stations'
"""
DUMP = """
c --- Write-only field values before original IPROG=0 weighted result
      if(k.le.2.and.
     1  ((i.eq.26.and.j.eq.26).or.(i.eq.1.and.j.eq.26).or.
     2   (i.eq.1.and.j.eq.1).or.(i.eq.13.and.j.eq.13)))then
         do d2ll=1,nst
            d2rank=0
            do d2mm=1,nk
               if(js(d2mm).eq.d2ll)d2rank=d2mm
            enddo
            write(91,*)d2call,i,j,k,d2ll,d2rank,
     1          rs(d2ll),xst(d2ll),yst(d2ll),stdat1(k,d2ll),
     2          wo(i,j,k),fgfld(i,j,k),field1(i,j,k),
     3          field2(i,j,k),r,wt3,nk,nintrp(k)
         enddo
      endif
"""


def instrument(source: Path, out: Path):
    raw = source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != BASE_SHA:
        raise RuntimeError("Original source hash changed; aborting instrumentation")
    lines = raw.decode("latin1").splitlines(keepends=True)
    for ln, expected in ANCHORS.items():
        assert lines[ln - 1].rstrip("\r\n") == expected, (ln, lines[ln - 1])
        if ln in BEFORE:
            assert lines[ln-2].rstrip("\r\n") == BEFORE[ln]
        if ln in AFTER:
            assert lines[ln].rstrip("\r\n") == AFTER[ln], (ln, lines[ln])
    additions = {8243: DECL, 8247: START, 8568: DUMP}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes("".join(
        line + additions.get(i, "") for i, line in enumerate(lines, 1)
    ).encode("latin1"))
    return hashlib.sha256(out.read_bytes()).hexdigest()


def parse_original(path: Path):
    columns = ("call", "i", "j", "k", "station", "selected_rank",
               "rs_km", "station_x_km", "station_y_km", "observed_mps",
               "wo", "step1_mps", "weighted_sum_before", "weight_sum_before",
               "r_km", "step_weight", "nk", "nintrp")
    rows = []
    with path.open() as stream:
        for line in stream:
            words = line.split()
            if len(words) != len(columns):
                raise RuntimeError(f"bad original trace line {len(rows)}: {line[:120]}")
            r = {name: (int(v) if n in (0,1,2,3,4,5,16,17) else float(v.replace("D","E")))
                 for n,(name,v) in enumerate(zip(columns,words))}
            rows.append(r)
    # 143 timesteps, U then V, two grid levels, four spatial samples, two stations.
    if len(rows) != 143*2*2*4*2:
        raise RuntimeError(f"expected 4576 original rows, got {len(rows)}")
    indices = {(r["call"],r["i"],r["j"],r["k"],r["station"]) for r in rows}
    if len(indices) != len(rows):
        raise RuntimeError("duplicate original INTER2 sampled record")
    for hour in range(1,144):
        for comp in (0,1):
            call = 2*(hour-1)+comp+1
            for (i,j) in SELECTED:
                for k in (1,2):
                    for station in (1,2):
                        if (call,i,j,k,station) not in indices:
                            raise RuntimeError(f"missing original index {(call,i,j,k,station)}")
    # All actual calls must have WO=1, since the case is IPROG=0.
    if any(abs(r["wo"]-1)>1e-6 for r in rows):
        raise RuntimeError("actual original WO not equal to 1: inspect original control route")
    if any(r["nk"]<1 for r in rows):
        raise RuntimeError("actual original INTER2 has a cell with no observations")
    return columns, rows


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--vendor", required=True, type=Path)
    p.add_argument("--case", required=True, type=Path)
    p.add_argument("--out", required=True, type=Path)
    p.add_argument("--compiler", default="/usr/bin/gfortran-11")
    args = p.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    vendor = args.vendor.resolve()
    if dgm.sha256_file(vendor / "calutils.for") != dgm.ORIGINAL_CALUTILS_SHA256:
        raise RuntimeError("original CALUTILS changed")
    modified = out/"calmet_trace_copy.for"
    instrumented_sha = instrument(vendor/"calmet.for", modified)
    aliases = dgm._alias_tree(vendor, out/"include-alias")
    original_bin = out/"bin/original.x"
    instrument_bin = out/"bin/instrumented.x"
    original_compile = dgm._build(args.compiler, vendor/"calmet.for", aliases, original_bin)
    trace_compile = dgm._build(args.compiler, modified, aliases, instrument_bin)
    staging = out/"staged-case"
    if staging.exists():
        shutil.rmtree(staging)
    dgm._stage_case(args.case.resolve(), staging)
    run_dir = staging/"run/met"
    original_run = dgm._run(original_bin, run_dir)
    trace = run_dir/"p6d_inter2_actual.txt"
    if trace.exists():
        raise RuntimeError("untouched original unexpectedly emitted diagnostic trace")
    instrumented_run = dgm._run(instrument_bin, run_dir)
    if original_run["met_dat_sha256"] != MET_SHA or instrumented_run["met_dat_sha256"] != MET_SHA:
        raise RuntimeError("Original/instrumented MET.DAT does not match frozen digest")
    if not trace.exists():
        raise RuntimeError("instrumented real CALMET did not emit INTER2 trace")
    headers, records = parse_original(trace)
    output = out/"original_inter2_actual.csv"
    with output.open("w", newline="") as handle:
        w = csv.DictWriter(handle, fieldnames=headers)
        w.writeheader()
        w.writerows(records)
    evidence = {
        "status": "ORIGINAL_INTER2_ACTUAL_WRITE_ONLY_PASS",
        "scope": "Hefei 2022, 143h, 4 points x 2 levels x 2 stations, U and V",
        "actual_call_count": 286,
        "original_source_sha256": BASE_SHA,
        "instrumented_source_sha256": instrumented_sha,
        "original_met_sha256": original_run["met_dat_sha256"],
        "instrumented_met_sha256": instrumented_run["met_dat_sha256"],
        "csv_sha256": dgm.sha256_file(output),
        "csv_rows": len(records),
        "selected_cells_1based": list(SELECTED),
        "columns": headers,
        "original_compiler_command": original_compile,
        "instrumented_compiler_command": trace_compile,
        "scientific_acceptance": "DIAGNOSTIC_NOT_P6D_FULL_PASS",
    }
    (out/"original_inter2_evidence.json").write_text(json.dumps(evidence,indent=2)+"\n")
    print(json.dumps({k:evidence[k] for k in ("status","actual_call_count","csv_rows",
                                             "csv_sha256","scientific_acceptance")},indent=2))


if __name__ == "__main__":
    main()
