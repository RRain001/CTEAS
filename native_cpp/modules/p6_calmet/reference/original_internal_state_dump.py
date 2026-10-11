#!/usr/bin/env python3
"""Original CALMET 6.5.0 internal-state dump (write-only instrumentation).

`CALMET.DAT` does not record the mixing-height state that the P6E reconstruction
still disagrees with at 1-2 % of cells: the convective mixing height `ZICONV`
and the Maul-Carson lapse-rate memory `DPTT`, plus the surface fluxes `QH`,
`USTAR`, `EL` the mixing-height model consumes, the air density `RHO` that turns
`QH` into the buoyancy production `wt`, the 2-D surface temperature `TEMP2D`,
the mixing height `ZI` and the convective velocity scale `WSTAR`.

The vendor tree is never modified. `instrument` copies the pinned original
`calmet.for` into a temporary file, appends a *write-only* call after each of the
five audited statements in `COMP` and one helper subroutine at the end of the
file, and fails closed if the vendor source hash or any source anchor moved.

`audit` then, on the build host, compiles the untouched original and the
instrumented copy under one compiler and one flag set, runs both on the frozen
2022 case with identical inputs, and requires the two produced `MET.DAT` files
to be byte identical (the instrumentation must be write-only). It writes a
binary dump plus JSON metadata (`metadata`) that names every field, its shape,
dtype, units, time label and insertion point.

Binary layout (little endian, written by `DGMSTATE` with gfortran's 4-byte
`INTEGER` and `REAL`):

    record := int32[8] = (kpt, nh, nyr, ijul, ihr, isec, nx, ny)
              followed by the point's fields, each nx*ny float32, i fastest

    kpt = 1  QH, USTAR, EL, RHO, TEMP2D      (after ELUSTR, before MIXHT)
    kpt = 2  ZICONV, DPTT                    (entering MIXHT: last hour's memory)
    kpt = 3  ZI, ZICONV, DPTT                (out of MIXHT, before AVEMIX)
    kpt = 4  ZI, ZICONV                      (after AVEMIX)
    kpt = 5  WSTAR, ZICONV                   (after WSTARR)

`nyr`/`ijul`/`ihr`/`isec` are the current timestep's beginning time: packed
`YYYYJJJHH` plus seconds, exactly the label `COMP` computes as `ndathrb`/`nsecb`.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

# --- pinned provenance -----------------------------------------------------
ORIGINAL_CALMET_SHA256 = "430381b4fb87f15127e20b13cc3e99be8a0e0fccf1d5821110af7f980558d134"
ORIGINAL_CALUTILS_SHA256 = "1e1fbad6444dcebbcaefed3f5f9a65e5ab0569d45fdf12ea973ca613b8d7cad1"
TARGET_MET_DAT_SHA256 = "b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30"

# The reference capture's CALMET recipe (scripts/capture_hefei_2022_reference_case.py).
COMMON_FFLAGS = ("-std=legacy", "-ffixed-line-length-none", "-fno-automatic",
                 "-fno-align-commons", "-fallow-argument-mismatch", "-O0")

# point -> (1-based vendor line, that line, the line before it, the line after).
# Every anchor is inside `subroutine comp`, in the hourly mixing-height block.
ANCHORS = {
    1: (4686, "      endif",
        "         endif",
        "c"),
    2: (4690, "c      ihrgmt=nhr+ibtz",
        "c --- (ihrgmt is GMT time for current hour -- can be >= 24)",
        "      if (itprog .eq. 0) then"),
    3: (4699, "      end if",
        "     1              ilandu,iwat1,iwat2,ldbhr,imixh,zi,ziconv)",
        ""),
    4: (4717, "      endif",
        "     1    v(1,1,ilevzi),zi,ziconv)",
        ""),
    5: (4750, "      endif",
        "     1   wstar)",
        "c"),
}

POINT_LABELS = {
    1: "after ELUSTR (QH/USTAR/EL) and the optional prints, before MIXHT",
    2: "immediately before the MIXHT/MIXHT2 call (memory entering the hour)",
    3: "immediately after MIXHT/MIXHT2, before AVEMIX",
    4: "immediately after the IAVEZI=1 AVEMIX averaging",
    5: "immediately after WSTARR",
}

POINT_FIELDS = {
    1: ["QH", "USTAR", "EL", "RHO", "TEMP2D"],
    2: ["ZICONV", "DPTT"],
    3: ["ZI", "ZICONV", "DPTT"],
    4: ["ZI", "ZICONV"],
    5: ["WSTAR", "ZICONV"],
}

FIELD_META = {
    "QH": ("W/m**2", "Sensible heat flux over land; buoyancy flux over water "
                     "(/METGRD/ QH)"),
    "USTAR": ("m/s", "Surface friction velocity (/METGRD/ USTAR)"),
    "EL": ("m", "Monin-Obukhov length, negative when unstable (/METGRD/ EL)"),
    "RHO": ("kg/m**3", "Air density at the lowest grid level (COMP local RHO)"),
    "TEMP2D": ("K", "2-D surface temperature field (COMP local TEMP2D)"),
    "ZI": ("m", "Mixing height, max(mechanical, ZICONV) limited to "
                "ZIMIN/ZIMAX (/METGRD/ ZI)"),
    "ZICONV": ("m", "Convective mixing height (COMP local ZICONV); 0 in the "
                    "stable branch of MIXHT"),
    "DPTT": ("K/m", "Potential-temperature lapse-rate memory of MIXHMC "
                    "(/TJMP/ DPTT); 0 in the stable branch and at program start"),
    "WSTAR": ("m/s", "Convective velocity scale (/METGRD/ WSTAR)"),
}

HELPER = """
c ---------------------------------------------------------------------
c --- DGM TRACK: write-only dump of CALMET internal state that the
c --- vendor model never records in CALMET.DAT. Present only in the
c --- temporary instrumented copy of this file; the vendor tree is
c --- untouched. Record layout is documented in
c --- native_cpp/modules/p6_calmet/reference/original_internal_state_dump.py
c ---------------------------------------------------------------------
      subroutine dgmstate(kpt,nh,nyr,ijul,ihr,isec,nx,ny,rho,temp2d,
     1                    ziconv)
      include 'params.met'
      integer kpt,nh,nyr,ijul,ihr,isec,nx,ny
      real rho(mxnx,mxny),temp2d(mxnx,mxny),ziconv(mxnx,mxny)
      logical ldgm
      save ldgm
      data ldgm/.false./
      include 'metgrd.met'
      common /tjump/ dptt(mxnx,mxny)
      integer i,j
c
      if(.not.ldgm)then
         open(97,file='dgm_state.bin',form='unformatted',
     1        access='stream',status='replace')
         ldgm=.true.
      endif
c
      write(97) kpt,nh,nyr,ijul,ihr,isec,nx,ny
c
      if(kpt.eq.1)then
         write(97) ((qh(i,j),i=1,nx),j=1,ny)
         write(97) ((ustar(i,j),i=1,nx),j=1,ny)
         write(97) ((el(i,j),i=1,nx),j=1,ny)
         write(97) ((rho(i,j),i=1,nx),j=1,ny)
         write(97) ((temp2d(i,j),i=1,nx),j=1,ny)
      else if(kpt.eq.2)then
         write(97) ((ziconv(i,j),i=1,nx),j=1,ny)
         write(97) ((dptt(i,j),i=1,nx),j=1,ny)
      else if(kpt.eq.3)then
         write(97) ((zi(i,j),i=1,nx),j=1,ny)
         write(97) ((ziconv(i,j),i=1,nx),j=1,ny)
         write(97) ((dptt(i,j),i=1,nx),j=1,ny)
      else if(kpt.eq.4)then
         write(97) ((zi(i,j),i=1,nx),j=1,ny)
         write(97) ((ziconv(i,j),i=1,nx),j=1,ny)
      else
         write(97) ((wstar(i,j),i=1,nx),j=1,ny)
         write(97) ((ziconv(i,j),i=1,nx),j=1,ny)
      endif
c
      return
      end
"""


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def instrument(vendor_dir: Path, out_dir: Path) -> dict:
    """Write the instrumented copy and the insertion report. Never edits vendor_dir."""
    source = vendor_dir / "calmet.for"
    actual = sha256_file(source)
    if actual != ORIGINAL_CALMET_SHA256:
        raise SystemExit(f"vendor calmet.for SHA256 drift: {actual}")
    calutils = vendor_dir / "calutils.for"
    calutils_actual = sha256_file(calutils)
    if calutils_actual != ORIGINAL_CALUTILS_SHA256:
        raise SystemExit(f"vendor calutils.for SHA256 drift: {calutils_actual}")

    lines = source.read_text().splitlines(keepends=True)
    for kpt, (line_no, expected, before, after) in ANCHORS.items():
        got = lines[line_no - 1].rstrip("\r\n")
        prev = lines[line_no - 2].rstrip("\r\n") if line_no > 1 else ""
        nxt = lines[line_no].rstrip("\r\n") if line_no < len(lines) else ""
        if got != expected or prev != before or nxt != after:
            raise SystemExit(
                f"COMP anchor drift at point {kpt} line {line_no}: got {got!r} "
                f"between {prev!r} and {nxt!r}")

    insertions = {}
    for kpt, (line_no, *_rest) in ANCHORS.items():
        insertions[line_no] = (
            f"c --- DGM write-only internal-state dump (point {kpt}: "
            f"{POINT_LABELS[kpt]})\n"
            f"      call dgmstate({kpt},nh,nyrb,njulb,nhrb,nsecb,nx,ny,rho,"
            f"temp2d,ziconv)\n")

    body = "".join(line + insertions.get(index, "")
                   for index, line in enumerate(lines, 1))
    instrumented = body + HELPER
    out_dir.mkdir(parents=True, exist_ok=True)
    target = out_dir / "calmet_instrumented.for"
    target.write_text(instrumented)

    # The instrumented source segment that carries every insertion: the COMP
    # mixing-height block, from the ELUSTR comment to the WSTARR block end, plus
    # the appended helper.
    segment = "".join(lines[4656:4751])
    report = {
        "status": "ORIGINAL_CALMET_WRITE_ONLY_INTERNAL_STATE_INSTRUMENTED",
        "why": ("CALMET.DAT records ZI/WSTAR/T-LEV but not the ZICONV/DPTT "
                "mixing-height memory nor QH/USTAR/EL/RHO/TEMP2D that produce "
                "them, so the residual P6E disagreement cannot be explained "
                "from the model output alone"),
        "no_vendor_source_edits": True,
        "vendor_source_sha256": {
            "calmet.for": actual,
            "calutils.for": calutils_actual,
        },
        "instrumented_sha256": {
            "calmet_instrumented.for": sha256_file(target),
        },
        "helper_subroutine": {
            "name": "DGMSTATE",
            "vendor_lines": len(lines) + 1,
            "sha256": sha256_bytes(HELPER.encode()),
            "unit": 97,
            "output_file": "dgm_state.bin",
            "writes_only": True,
        },
        "points": [
            {
                "kpt": kpt,
                "vendor_line": line_no,
                "when": POINT_LABELS[kpt],
                "fields": POINT_FIELDS[kpt],
                "inserted_text_sha256": sha256_bytes(
                    insertions[line_no].encode()),
            }
            for kpt, (line_no, *_rest) in sorted(ANCHORS.items())
        ],
        "instrumented_source_segment": {
            "vendor_lines": [4657, 4751],
            "sha256": sha256_bytes(segment.encode()),
        },
        "record_layout": {
            "header": ["kpt", "nh", "nyr", "ijul", "ihr", "isec", "nx", "ny"],
            "header_dtype": "int32 little endian",
            "field_dtype": "float32 little endian",
            "field_order": "i fastest: element = (j-1)*nx + i, i=1..nx, j=1..ny",
        },
    }
    (out_dir / "dgm-instrumentation.json").write_text(
        json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return report


def _build(compiler: str, source: Path, include_dir: Path,
           target: Path) -> list[str]:
    """Compile one vendor binary.

    `calmet.for` textually INCLUDEs `calutils.for` and `coordlib.for`, so those
    must not also be passed as separate translation units (duplicate symbols).
    """
    command = [compiler, *COMMON_FFLAGS, f"-I{include_dir}", str(source),
               "-o", str(target)]
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        target.unlink()
    subprocess.run(command, check=True, cwd=str(target.parent))
    target.chmod(0o755)
    return command


def _alias_tree(vendor_dir: Path, alias_dir: Path) -> Path:
    """Lower-case symlink alias tree: the vendor sources open INCLUDE files in
    lower case while the tracked names are upper case."""
    alias_dir.mkdir(parents=True, exist_ok=True)
    for item in vendor_dir.iterdir():
        if not item.is_file():
            continue
        link = alias_dir / item.name.lower()
        if not link.exists():
            link.symlink_to(item)
    return alias_dir


def _stage_case(case_dir: Path, root: Path) -> dict:
    mapping = {
        "geo/makegeo/geo.dat": case_dir / "geo/makegeo/geo.dat",
        "met/surface/final/surf.dat": case_dir / "met/surface/final/surf.dat",
        "met/upperair/final/up_58424.dat":
            case_dir / "met/upperair/final/up_58424.dat",
        "run/met/met.inp": case_dir / "run/met/met.inp",
    }
    staged = {}
    for relative, source in mapping.items():
        if not source.is_file():
            raise SystemExit(f"missing frozen case input: {source}")
        destination = root / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        staged[relative] = {"sha256": sha256_file(source),
                            "bytes": source.stat().st_size}
    return staged


def _run(binary: Path, run_dir: Path) -> dict:
    meta = run_dir / "met.dat"
    listing = run_dir / "met.lst"
    log = run_dir / "met_runtime.log"
    for stale in (meta, listing, log, run_dir / "dgm_state.bin"):
        if stale.exists():
            stale.unlink()
    with log.open("w") as handle:
        proc = subprocess.run([str(binary), "met.inp"], cwd=str(run_dir),
                              stdout=handle, stderr=subprocess.STDOUT)
    if proc.returncode != 0:
        raise SystemExit(f"met.x failed with {proc.returncode}; see {log}")
    if not meta.is_file():
        raise SystemExit(f"met.x produced no met.dat; see {log}")
    return {
        "command": [str(binary), "met.inp"],
        "cwd": str(run_dir),
        "returncode": proc.returncode,
        "met_dat_sha256": sha256_file(meta),
        "met_dat_bytes": meta.stat().st_size,
    }


def audit(args: argparse.Namespace) -> dict:
    vendor = args.vendor.resolve()
    case = args.case.resolve()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)

    instrumentation = instrument(vendor, out)
    alias = _alias_tree(vendor, out / "include-alias")

    original_x = out / "bin" / "met-original.x"
    instrumented_x = out / "bin" / "met-instrumented.x"
    original_command = _build(args.compiler, vendor / "calmet.for", alias,
                              original_x)
    instrumented_command = _build(args.compiler, out / "calmet_instrumented.for",
                                  alias, instrumented_x)

    staging = out / "case"
    if staging.exists():
        shutil.rmtree(staging)
    staged = _stage_case(case, staging)
    run_dir = staging / "run" / "met"

    original_run = _run(original_x, run_dir)
    original_dump = run_dir / "dgm_state.bin"
    if original_dump.exists():
        raise SystemExit("the untouched original wrote a dump file: the "
                         "baseline binary is not the untouched source")
    instrumented_run = _run(instrumented_x, run_dir)
    dump = run_dir / "dgm_state.bin"
    if not dump.is_file():
        raise SystemExit("the instrumented run wrote no dgm_state.bin")

    compiler_version = subprocess.run([args.compiler, "--version"],
                                      capture_output=True, text=True,
                                      check=True).stdout.splitlines()[0]
    identical = original_run["met_dat_sha256"] == instrumented_run["met_dat_sha256"]
    matches_target = instrumented_run["met_dat_sha256"] == TARGET_MET_DAT_SHA256
    evidence = {
        "status": ("ORIGINAL_MET_DAT_IDENTICAL" if identical
                   else "INSTRUMENTATION_CHANGED_MET_DAT"),
        "compiler": args.compiler,
        "compiler_version": compiler_version,
        "common_flags": list(COMMON_FFLAGS),
        "include_handling": "lower-case symlink alias tree per vendor directory",
        "build_commands": {
            "original": original_command,
            "instrumented": instrumented_command,
        },
        "case": {
            "path": str(case),
            "inputs": staged,
        },
        "runs": {"original": original_run, "instrumented": instrumented_run},
        "met_dat_identical": identical,
        "met_dat_matches_frozen_reference": matches_target,
        "frozen_reference_sha256": TARGET_MET_DAT_SHA256,
        "instrumentation": instrumentation,
    }
    (out / "dgm-audit.json").write_text(json.dumps(evidence, indent=2) + "\n")
    if not identical:
        raise SystemExit("instrumentation is not write-only: MET.DAT changed")
    if not matches_target:
        print(f"WARNING: MET.DAT {instrumented_run['met_dat_sha256']} does not "
              f"match the frozen reference {TARGET_MET_DAT_SHA256}")
    print(json.dumps({k: evidence[k] for k in
                      ("status", "met_dat_identical",
                       "met_dat_matches_frozen_reference")}, indent=2))
    return evidence


def _read_records(path: Path):
    blob = path.read_bytes()
    offset = 0
    records = []
    while offset < len(blob):
        if offset + 32 > len(blob):
            raise SystemExit(f"truncated record header at byte {offset}")
        header = struct.unpack_from("<8i", blob, offset)
        kpt, nh, nyr, ijul, ihr, isec, nx, ny = header
        if kpt not in POINT_FIELDS:
            raise SystemExit(f"unknown point {kpt} at byte {offset}")
        count = len(POINT_FIELDS[kpt])
        size = count * nx * ny * 4
        start = offset + 32
        if start + size > len(blob):
            raise SystemExit(f"truncated field block at byte {offset}")
        values = struct.unpack_from(f"<{count * nx * ny}f", blob, start)
        fields = {}
        for index, name in enumerate(POINT_FIELDS[kpt]):
            base = index * nx * ny
            fields[name] = values[base:base + nx * ny]
        records.append({
            "kpt": kpt, "nh": nh, "nyr": nyr, "ijul": ijul, "ihr": ihr,
            "isec": isec, "nx": nx, "ny": ny, "byte_offset": offset,
            "fields": fields,
        })
        offset = start + size
    return records


def _stats(values):
    return {"min": min(values), "max": max(values),
            "mean": sum(values) / len(values)}


def metadata(args: argparse.Namespace) -> dict:
    dump = args.dump.resolve()
    if not dump.is_file():
        raise SystemExit(f"no dump at {dump}")
    records = _read_records(dump)
    hours = {}
    for record in records:
        hour = hours.setdefault(record["nh"], {})
        if record["kpt"] in hour:
            raise SystemExit(f"duplicate point {record['kpt']} for hour {record['nh']}")
        hour[record["kpt"]] = record
    missing = [(nh, kpt) for nh, hour in hours.items()
               for kpt in POINT_FIELDS if kpt not in hour]
    if missing:
        raise SystemExit(f"incomplete hour records: {missing[:8]}")
    labels = []
    for nh in sorted(hours):
        hour = hours[nh]
        keys = {(hour[kpt]["nyr"], hour[kpt]["ijul"], hour[kpt]["ihr"],
                 hour[kpt]["isec"]) for kpt in POINT_FIELDS}
        if len(keys) != 1:
            raise SystemExit(f"hour {nh} carries inconsistent time labels")
        nyr, ijul, ihr, isec = keys.pop()
        labels.append({"hour_index": nh, "packed": nyr * 100000 + ijul * 100 + ihr,
                       "seconds": isec, "year": nyr, "julian_day": ijul,
                       "hour_utc": ihr})
    if args.dgrid_m is None or args.station_cell is None:
        raise SystemExit("--dgrid-m and --station-cell are required metadata "
                         "inputs (station cell as 1-based 'i,j')")
    dgrid = args.dgrid_m
    station_i, station_j = (int(part) for part in args.station_cell.split(","))
    nx = records[0]["nx"]
    ny = records[0]["ny"]

    neighbourhood = []
    for nh in sorted(hours):
        for kpt in sorted(POINT_FIELDS):
            record = hours[nh][kpt]
            cells = []
            for j in range(station_j - 2, station_j + 1):
                for i in range(station_i - 2, station_i + 1):
                    if not (1 <= i <= nx and 1 <= j <= ny):
                        continue
                    index = (j - 1) * nx + i - 1
                    cells.append({
                        "i": i, "j": j,
                        "values": {name: record["fields"][name][index]
                                   for name in POINT_FIELDS[kpt]},
                    })
            neighbourhood.append({"hour_index": nh, "kpt": kpt,
                                  "packed": hours[nh][1]["nyr"] * 100000
                                            + hours[nh][1]["ijul"] * 100
                                            + hours[nh][1]["ihr"],
                                  "cells": cells})

    summary = {}
    for kpt, names in POINT_FIELDS.items():
        summary[str(kpt)] = {
            name: {
                "grid": _stats([record["fields"][name][index]
                                for record in records if record["kpt"] == kpt
                                for index in range(nx * ny)]),
                "station_3x3": _stats([
                    record["fields"][name][(j - 1) * nx + i - 1]
                    for record in records if record["kpt"] == kpt
                    for j in range(max(1, station_j - 1), station_j + 2)
                    for i in range(max(1, station_i - 1), station_i + 2)]),
            }
            for name in names
        }

    neighbourhood_path = args.out / "dgm_station_neighbourhood.json"
    neighbourhood_path.write_text(json.dumps({
        "what": "station-cell 3x3 neighbourhood of the CALMET internal state",
        "station_cell": {"i": station_i, "j": station_j,
                         "orientation": "1-based CALMET grid indices, i east, "
                                        "j north, cell centre of the surface "
                                        "station's nearest grid point"},
        "records": neighbourhood,
    }, indent=2) + "\n")

    payload = {
        "what": ("internal state of the UNMODIFIED original CALMET 6.5.0 "
                 "(level 150223) that CALMET.DAT does not record, exported by "
                 "write-only instrumentation and a rerun of the frozen Hefei "
                 "2022 case"),
        "why": ("P6E disagrees with the frozen reference on ZI, WSTAR and "
                "T-LEV 5/6/7 in ~1.2-1.4 % of cells; ZICONV, DPTT and QH are "
                "not representable in CALMET.DAT 2.1 (the writer rejects them "
                "as file labels), so the residual cannot be diagnosed from the "
                "model output alone"),
        "source": {
            "subroutine": "COMP (@@ lines below) and the appended DGMSTATE",
            "vendor_file": "engine/vendor/src/CALMET_v6.5.0_L150223/calmet.for",
            "vendor_sha256": ORIGINAL_CALMET_SHA256,
            "instrumentation": "native_cpp/modules/p6_calmet/reference/"
                               "original_internal_state_dump.py",
            "insertion_points": {
                str(kpt): {"vendor_line": line_no, "when": POINT_LABELS[kpt],
                           "fields": POINT_FIELDS[kpt]}
                for kpt, (line_no, *_rest) in sorted(ANCHORS.items())
            },
            "write_only": ("both the untouched original and the instrumented "
                           "copy were compiled and run on identical inputs; "
                           "MET.DAT must hash identically (see dgm-audit.json)"),
        },
        "binary": {
            "path": str(dump),
            "bytes": dump.stat().st_size,
            "sha256": sha256_file(dump),
            "byte_order": "little endian",
            "header": "int32[8] = kpt, nh, nyr, ijul, ihr, isec, nx, ny",
            "field_dtype": "float32",
            "field_order": ("i fastest, then j: element (j-1)*nx+i for 1-based "
                            "i=1..nx east, j=1..ny north; hour major, then "
                            "point kpt=1..5"),
            "bytes_per_field": nx * ny * 4,
            "hours": len(hours),
            "records": len(records),
        },
        "grid": {
            "nx": nx, "ny": ny, "dgrid_m": dgrid,
            "registration": "cell centres, cell (i,j) centred at 0.5*dgrid "
                            "inside the cell; CALMET.DAT /GRID/ XORIGR/YORIGR "
                            "is the south-west corner",
            "station_cell": {"i": station_i, "j": station_j},
            "station_cell_index_0based_i_fastest": (station_j - 1) * nx
                                                   + station_i - 1,
            "neighbourhood_file": str(neighbourhood_path),
            "neighbourhood_sha256": sha256_file(neighbourhood_path),
        },
        "fields": {name: {"units": units, "meaning": meaning}
                   for name, (units, meaning) in FIELD_META.items()},
        "missing_sentinel": {
            "applied": False,
            "calmet_dat_marker": -7777.0,
            "note": ("CALMET.DAT 2.1 encodes missing grid values as -7777.0, "
                     "but every dumped array is fully populated at the dump "
                     "points, so no sentinel value is inserted or interpreted "
                     "here; a reader must not special-case -7777.0 in these "
                     "arrays"),
        },
        "time": {
            "label": "packed YYYYJJJHH (year, zero-padded Julian day, hour UTC)",
            "seconds": "timestep beginning seconds; 0 for this hourly case",
            "hour_index": "nh, the COMP hour counter, 1..143 in order",
            "begin_utc": "2022-01-01T00:00:00Z",
            "end_utc": "2022-01-06T23:00:00Z",
            "hours": labels,
        },
        "statistics": summary,
    }
    metadata_path = args.out / "dgm_state.meta.json"
    metadata_path.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps({
        "metadata": str(metadata_path),
        "metadata_sha256": sha256_file(metadata_path),
        "neighbourhood": str(neighbourhood_path),
        "neighbourhood_sha256": sha256_file(neighbourhood_path),
        "dump_sha256": payload["binary"]["sha256"],
        "records": len(records),
        "hours": len(hours),
    }, indent=2))
    return payload


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="mode", required=True)

    one = sub.add_parser("instrument", help="write the instrumented copy")
    one.add_argument("--vendor", type=Path, required=True)
    one.add_argument("--out", type=Path, required=True)

    two = sub.add_parser("audit", help="build, run and prove write-only")
    two.add_argument("--vendor", type=Path, required=True)
    two.add_argument("--case", type=Path, required=True)
    two.add_argument("--out", type=Path, required=True)
    two.add_argument("--compiler", default="/usr/bin/gfortran-11")

    three = sub.add_parser("metadata", help="dump + metadata JSON")
    three.add_argument("--dump", type=Path, required=True)
    three.add_argument("--out", type=Path, required=True)
    three.add_argument("--dgrid-m", type=float, required=True)
    three.add_argument("--station-cell", required=True,
                       help="1-based 'i,j' of the station grid cell")

    args = parser.parse_args()
    if args.mode == "instrument":
        instrument(args.vendor.resolve(), args.out.resolve())
    elif args.mode == "audit":
        audit(args)
    else:
        args.out.mkdir(parents=True, exist_ok=True)
        metadata(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
