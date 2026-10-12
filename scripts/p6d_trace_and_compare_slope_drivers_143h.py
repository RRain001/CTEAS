#!/usr/bin/env python3
"""Source-locked 143h audit of inputs actually present at the CALMET DIAGNO SLOPE call.

The original CALMET is not edited. A third, write-only instrumented copy is
compiled alongside the already audited original/source-instrumented pair.
The additional trace samples QH/TEMP2D/TERSET immediately after SLOPE and
before UG/VG addition; every reference MET.DAT must remain byte identical.
This is an upstream driver audit, not a P6D or U/V/W acceptance claim.
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "native_cpp/modules/p6_calmet/reference"))
import original_internal_state_dump as dgm

NX = NY = 50
NH = 143
CELLS = NX * NY
FIELDS = ("QH", "TEMP2D", "HMAX", "HMIN", "DISTHMAX", "DISTHMIN", "HTOPO")
NATIVE = {
    "QH": "qh_{label}.f32",
    "TEMP2D": "temp2d_{label}.f32",
    "HMAX": "terset_hmax.f32",
    "HMIN": "terset_hmin.f32",
    "DISTHMAX": "terset_disthmax.f32",
    "DISTHMIN": "terset_disthmin.f32",
}
# Diagnostic thresholds, declared before seeing the 143-hour values.
TOL = {"QH": 2e-4, "TEMP2D": 1e-4,
       "HMAX": 2e-5, "HMIN": 2e-5,
       "DISTHMAX": 1e-2, "DISTHMIN": 1e-2}

# Uses the same COMMON grid/metgrd variables as literal CALMET SLOPE.
# Explicit arguments are the DIAGNO-local arrays, not reconstructed values.
HELPER = """
c --- Separate write-only original SLOPE driver trace; unit 94
      subroutine dgmslope(t2,hx,topo)
      include 'params.met'
      include 'gen.met'
      include 'grid.met'
      include 'metgrd.met'
      real t2(mxnx,mxny),hx(mxnx,mxny),topo(mxnx,mxny)
      integer i,j
      logical opened
      save opened
      data opened/.false./
      if(.not.opened)then
         open(94,file='dgm_slope_drivers.bin',form='unformatted',
     1        access='stream',status='replace')
         opened=.true.
      endif
      write(94) 11,ndathrb,nsecb,nx,ny
      write(94) ((qh(i,j),i=1,nx),j=1,ny)
      write(94) ((t2(i,j),i=1,nx),j=1,ny)
      write(94) ((hx(i,j),i=1,nx),j=1,ny)
      write(94) ((hmin(i,j),i=1,nx),j=1,ny)
      write(94) ((disthmax(i,j),i=1,nx),j=1,ny)
      write(94) ((disthmin(i,j),i=1,nx),j=1,ny)
      write(94) ((topo(i,j),i=1,nx),j=1,ny)
      return
      end
"""

def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1048576), b""):
            h.update(chunk)
    return h.hexdigest()

def install_write_only_trace(original: Path, out: Path) -> None:
    body = original.read_text()
    anchor = "      call dgmfld(7)\n"
    if body.count(anchor) != 1 or "      call dgmslope(" in body:
        raise RuntimeError("source SLOPE driver anchor drift or repeated instrumentation")
    # Point 7 sits immediately after CALMET's SLOPE call, before its UG/VG addition.
    body = body.replace(anchor, anchor +
        "      call dgmslope(temp2d,hmax,htopo)\n", 1)
    out.write_text(body + HELPER)

def read_native(path: Path) -> tuple[float, ...]:
    raw = path.read_bytes()
    if len(raw) != CELLS * 4:
        raise RuntimeError(f"native field wrong byte count: {path}")
    val = struct.unpack("<2500f", raw)
    if not all(math.isfinite(v) and abs(v) < 999998 for v in val):
        raise RuntimeError(f"native field nonfinite/missing: {path}")
    return val

def read_source(path: Path):
    raw = path.read_bytes()
    recsize = 20 + len(FIELDS) * CELLS * 4
    if len(raw) != NH * recsize:
        raise RuntimeError(f"source slope driver stream length mismatch: {len(raw)}")
    for h in range(NH):
        off = h * recsize
        kpt, packed, sec, nx, ny = struct.unpack_from("<5i", raw, off)
        clock = dt.datetime(2022, 1, 1) + dt.timedelta(hours=h)
        expected = clock.year * 100000 + clock.timetuple().tm_yday * 100 + clock.hour
        if (kpt, packed, sec, nx, ny) != (11, expected, 0, NX, NY):
            raise RuntimeError(f"original source hour/shape/order mismatch at hour {h+1}")
        values = struct.unpack_from("<17500f", raw, off + 20)
        if not all(math.isfinite(v) and abs(v) < 999998 for v in values):
            raise RuntimeError(f"original nonfinite/sentinel slope driver at hour {h+1}")
        yield {name: values[i*CELLS:(i+1)*CELLS] for i,name in enumerate(FIELDS)}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--original-dir", required=True, type=Path)
    ap.add_argument("--vendor", required=True, type=Path)
    ap.add_argument("--case", required=True, type=Path)
    ap.add_argument("--candidate", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--compiler", default="gfortran-11")
    ap.add_argument("--require-numeric-parity", action="store_true")
    args = ap.parse_args()

    audit = json.loads((args.original_dir / "dgm-audit.json").read_text())
    if (audit.get("status") != "ORIGINAL_MET_DAT_IDENTICAL"
        or not audit.get("met_dat_identical")
        or not audit.get("met_dat_matches_frozen_reference")
        or audit.get("frozen_reference_sha256") != dgm.TARGET_MET_DAT_SHA256
        or sha(args.vendor / "calmet.for") != dgm.ORIGINAL_CALMET_SHA256):
        raise RuntimeError("baseline original source or frozen MET.DAT not verified")
    baseline = args.original_dir / "calmet_instrumented.for"
    if sha(baseline) != audit["instrumentation"]["instrumented_sha256"]["calmet_instrumented.for"]:
        raise RuntimeError("baseline write-only instrumented source does not match audit")

    root = args.out.parent / "slope-driver-original-build"
    root.mkdir(parents=True, exist_ok=True)
    copied = root / "calmet_slope_drivers_instrumented.for"
    install_write_only_trace(baseline, copied)
    exe = root / "bin" / "met-original-slope-drivers.x"
    alias = args.original_dir / "include-alias"
    dgm._build(args.compiler, copied, alias, exe)
    staged_root = root / "case"
    dgm._stage_case(args.case.resolve(), staged_root)
    run_dir = staged_root / "run/met"
    run = dgm._run(exe, run_dir)
    stream = run_dir / "dgm_slope_drivers.bin"
    if not stream.is_file() or run["met_dat_sha256"] != dgm.TARGET_MET_DAT_SHA256:
        raise RuntimeError("additional original trace modified MET.DAT or has no driver stream")

    labels = [(dt.datetime(2022,1,1) + dt.timedelta(hours=h+1)).strftime("%Y-%m-%d_%H")
              for h in range(NH)]
    if (args.candidate / "produced_hours.txt").read_text().split() != labels:
        raise RuntimeError("native and original hourly labels differ")
    stats = {name: {"compared": 0, "exceedances": 0, "max_abs": 0.0,
                    "sum_sq": 0.0, "worst": None} for name in NATIVE}
    for h, source in enumerate(read_source(stream)):
        for name, pattern in NATIVE.items():
            cand = read_native(args.candidate / pattern.format(label=labels[h]))
            stat = stats[name]
            for idx, (reference, actual) in enumerate(zip(source[name], cand)):
                delta = abs(reference - actual)
                stat["compared"] += 1
                stat["sum_sq"] += delta * delta
                if delta > TOL[name]:
                    stat["exceedances"] += 1
                if delta > stat["max_abs"]:
                    stat["max_abs"] = delta
                    stat["worst"] = {
                        "hour_end": labels[h], "i": idx % NX + 1,
                        "j": idx // NX + 1,
                        "original": reference, "native": actual,
                        "abs_error": delta}
    for name, stat in stats.items():
        if stat["compared"] != NH * CELLS:
            raise RuntimeError(f"incomplete source input coverage: {name}")
        stat["rms"] = math.sqrt(stat.pop("sum_sq") / stat["compared"])
    verdict = ("PASS_SLOPE_DRIVER_INPUT_PARITY" if
               all(s["exceedances"] == 0 for s in stats.values()) else
               "FAIL_SLOPE_DRIVER_INPUT_PARITY")
    report = {
        "trace_status": "PASS_ORIGINAL_SLOPE_INPUT_TRACE_COMPLETE",
        "scientific_verdict": verdict, "hours": NH, "grid": [NX, NY],
        "source_sha256": dgm.ORIGINAL_CALMET_SHA256,
        "frozen_met_sha256": dgm.TARGET_MET_DAT_SHA256,
        "baseline_instrumented_sha256": sha(baseline),
        "extra_instrumented_sha256": sha(copied),
        "extra_trace_sha256": sha(stream),
        "extra_run_met_sha256": run["met_dat_sha256"],
        "instrumentation": "DIAGNO after SLOPE, before UG/VG addition; QH, TEMP2D, HMAX, HMIN, DISTHMAX, DISTHMIN, HTOPO",
        "reference_scope": "REAL*4 original source, no oracle input fed to C++; no U/V/W acceptance",
        "native_scope": "autonomous per-hour QH/TEMP2D and TERSET arrays, preexisting C++ producer",
        "absolute_tolerance": TOL, "fields": stats,
        "total_compared": NH * CELLS * len(NATIVE),
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"trace_status": report["trace_status"],
                      "scientific_verdict": verdict,
                      "summary": {k: {"exceedances": v["exceedances"],
                                      "max_abs": v["max_abs"],
                                      "worst": v["worst"]} for k,v in stats.items()}},
                     indent=2))
    if args.require_numeric_parity and verdict != "PASS_SLOPE_DRIVER_INPUT_PARITY":
        raise SystemExit(1)

if __name__ == "__main__":
    main()
