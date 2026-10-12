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
    5593: "      endif",
    5640: "C",
    6062: "",
    36018: "         nstat = nssta + nowsta + nusta",
    5726: "C",
    5732: "  300 CONTINUE",
    8243: "      double precision wt2,wt3",
    8247: "      DATA npts/4/",
    8568: "      wt3 = 1./(R**2)",
}
BEFORE = {5593: "         end if",
          5640: "",
          6062: "     2      iwat1,iwat2,vdat,noobs)",
          36018: "c        --total number of stations (sfc+upper air)",
          5726: "C     ADD THE SLOPE FLOW COMPONENT TO THE HORIZONTAL WIND FIELD",
          8243: "      DOUBLE PRECISION WT"}
AFTER = {6062: "      ELSE", 5593: "C", 5640: "C ---- APPLY THE MELSAR FROUDE NUMBER ADJUSTMENT",
         36018: "", 8247: "C", 8568: "c"}

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


PRETAIL_HELPER = """
c ----------------------------------------------------------------------
c --- Selected original DIAGNO immediately after its two INTER2 calls:
c --- store raw U/V before any SMOOTH, boundary or DIVCEL correction.
c --- Write-only temporary vendor copy, independent actual case input.
c ----------------------------------------------------------------------
      subroutine d2inter2pretail(u,v,nx,ny,nz)
      include 'params.met'
      integer nx,ny,nz,i,j,k,nhours
      real u(mxnx,mxny,mxnz),v(mxnx,mxny,mxnz)
      save nhours
      data nhours/0/
      nhours=nhours+1
      if(nhours.eq.1)then
          open(95,file='p6d_inter2_pretail.f32',
     1         form='unformatted',access='stream',status='replace')
      endif
      if(nx.ne.50.or.ny.ne.50.or.nz.ne.9)
     1  stop 'D2 INTER2 pre-tail geometry changed'
      write(95) (((u(i,j,k),i=1,nx),j=1,ny),k=1,nz)
      write(95) (((v(i,j,k),i=1,nx),j=1,ny),k=1,nz)
      return
      end
"""


GAMMA_HELPER = """
c ----------------------------------------------------------------------
c --- P6D D-2: original 50x50 GAMMA at FRADJ boundary (write-only)
c ----------------------------------------------------------------------
      subroutine d2gamdump(gamma,nx,ny)
      include 'params.met'
      integer nx,ny,i,j
      real gamma(mxnx,mxny)
      logical first
      save first
      data first/.true./
      if(first)then
         open(94,file='p6d_gamma_original.f32',form='unformatted',
     1        access='stream',status='replace')
         first=.false.
      endif
      write(94) ((gamma(i,j),i=1,nx),j=1,ny)
      return
      end
"""


WIND1_INPUT_HELPER = """
c ----------------------------------------------------------------------
c --- P6D D-2: original US/VS as WIND1 enters, before neutral SIMILT
c ----------------------------------------------------------------------
      subroutine d2windraw(us,vs,nstat)
      include 'params.met'
      integer nstat,k,l,nhcall
      real us(mxnz,mxwnd),vs(mxnz,mxwnd)
      save nhcall
      data nhcall/0/
      nhcall=nhcall+1
      if(nhcall.eq.1)then
         open(93,file='p6d_wind1_input_original.txt',
     1        form='formatted',status='replace')
      endif
      if(nstat.ne.2)stop 'D2 WIND1 requires two input stations'
      do l=1,2
         do k=1,2
            write(93,*)nhcall,l,k,us(k,l),vs(k,l)
         enddo
      enddo
      return
      end
"""


UPSTREAM_HELPER = """
c ----------------------------------------------------------------------
c --- P6D D-2 original-only in-chain SLOPE boundary state dump. This is
c --- appended to the TEMPORARY copy, never the unchanged vendor source.
c ----------------------------------------------------------------------
      subroutine d2upstream(kpt,packed,secs,ug,vg,qh,temp2d,
     1                      nx,ny,nz)
      include 'params.met'
      integer kpt,packed,secs,nx,ny,nz,i,j,k,ind
      integer ic(4),jc(4)
      real ug(mxnx,mxny,mxnz),vg(mxnx,mxny,mxnz)
      real qh(mxnx,mxny),temp2d(mxnx,mxny)
      logical first
      save first
      data first/.true./
      data ic/26,1,1,13/,jc/26,26,1,13/
      if(first)then
          open(92,file='p6d_upstream_actual.txt',
     1         form='formatted',status='replace')
          first=.false.
      endif
      do ind=1,4
          i=ic(ind)
          j=jc(ind)
          do k=1,2
              if(kpt.eq.7)then
                  write(92,*)packed,secs,kpt,i,j,k,
     1                       ug(i,j,k),vg(i,j,k),-999.,-999.
              else
                  write(92,*)packed,secs,kpt,i,j,k,
     1                       ug(i,j,k),vg(i,j,k),
     2                       qh(i,j),temp2d(i,j)
              endif
          enddo
      enddo
      return
      end
"""


def parse_upstream(path: Path):
    import datetime as dt
    names=("hour_begin_packed","seconds","kpt","i","j","k","u","v","qh","temp2d")
    rows=[]
    with path.open() as handle:
        for line in handle:
            w=line.split()
            if len(w)!=10:raise RuntimeError("bad original upstream line")
            record={n:(int(v) if ix<6 else float(v.replace("D","E")))
                    for ix,(n,v) in enumerate(zip(names,w))}
            rows.append(record)
    if len(rows)!=143*3*4*2:
        raise RuntimeError(f"upstream expected 3432 rows got {len(rows)}")
    keys=set()
    per_hour={}
    for r in rows:
        packed=r["hour_begin_packed"]
        year,jday,hour=packed//100000,(packed//100)%1000,packed%100
        if year!=2022 or jday<1 or jday>6 or hour<0 or hour>23 or r["seconds"]!=0:
            raise RuntimeError(f"bad timestamp {packed}")
        label=(dt.datetime(2022,1,1)+dt.timedelta(days=jday-1,hours=hour+1)).strftime("%Y-%m-%d_%H")
        r["label"]=label
        key=(label,r["kpt"],r["i"],r["j"],r["k"])
        if key in keys or r["kpt"] not in (7,8,9) or (r["i"],r["j"]) not in SELECTED or r["k"] not in (1,2):
            raise RuntimeError(f"bad/duplicate original upstream record {key}")
        keys.add(key)
        per_hour.setdefault(label,set()).add((r["kpt"],r["i"],r["j"],r["k"]))
    if len(per_hour)!=143 or any(len(v)!=24 for v in per_hour.values()):
        raise RuntimeError("incomplete original upstream hourly grid")
    return rows


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
    additions = {6062: "         call d2inter2pretail(u,v,nx,ny,nz)\n",
                 36018: "      call d2windraw(us,vs,nstat)\n",
                 5640: "      call d2gamdump(gamma,nx,ny)\n",
                 5593: "      call d2upstream(7,ndathrb,nsecb,ug,vg,qh,temp2d,\n     1                nx,ny,nz)\n",
                 5726: "      call d2upstream(8,ndathrb,nsecb,ug,vg,qh,temp2d,\n     1                nx,ny,nz)\n",
                 5732: "      call d2upstream(9,ndathrb,nsecb,ug,vg,qh,temp2d,\n     1                nx,ny,nz)\n",
                 8243: DECL, 8247: START, 8568: DUMP}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(("".join(
        line + additions.get(i, "") for i, line in enumerate(lines, 1)
    ) + UPSTREAM_HELPER + WIND1_INPUT_HELPER + GAMMA_HELPER + PRETAIL_HELPER).encode("latin1"))
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
    if (trace.exists() or (run_dir/"p6d_upstream_actual.txt").exists() or
        (run_dir/"p6d_wind1_input_original.txt").exists() or
        (run_dir/"p6d_gamma_original.f32").exists() or
        (run_dir/"p6d_inter2_pretail.f32").exists()):
        raise RuntimeError("untouched original unexpectedly emitted diagnostic trace")
    instrumented_run = dgm._run(instrument_bin, run_dir)
    if original_run["met_dat_sha256"] != MET_SHA or instrumented_run["met_dat_sha256"] != MET_SHA:
        raise RuntimeError("Original/instrumented MET.DAT does not match frozen digest")
    if not trace.exists():
        raise RuntimeError("instrumented real CALMET did not emit INTER2 trace")
    pretail=run_dir/"p6d_inter2_pretail.f32"
    expected_pretail_bytes=143*2*50*50*9*4
    if not pretail.is_file() or pretail.stat().st_size!=expected_pretail_bytes:
        raise RuntimeError("source-verified original INTER2 pre-tail 3D dump incomplete")
    raw_pretail=pretail.read_bytes()
    import datetime as dt
    stage_labels=[(dt.datetime(2022,1,1)+dt.timedelta(hours=h)).strftime("%Y-%m-%d_%H")
                  for h in range(1,144)]
    if len(stage_labels)!=143:raise RuntimeError("pre-tail hour labels incomplete")
    selected_pretail=(1,2,3,81,82,83)
    pretail_artifacts={}
    field_bytes=50*50*9*4
    for hour in selected_pretail:
        label=stage_labels[hour-1]
        for comp,axis in enumerate(("u","v")):
            offset=((hour-1)*2+comp)*field_bytes
            segment=raw_pretail[offset:offset+field_bytes]
            import math, struct
            values=struct.unpack("<22500f",segment)
            if not all(math.isfinite(v) and abs(v)<10000 for v in values):
                raise RuntimeError("original pre-tail contains invalid wind")
            output_path=out/("original_inter2_pretail_"+axis+"_"+label+".f32")
            output_path.write_bytes(segment)
            pretail_artifacts[label+"/"+axis]=dgm.sha256_file(output_path)
    gamma_raw=run_dir/"p6d_gamma_original.f32"
    if not gamma_raw.is_file() or gamma_raw.stat().st_size!=143*50*50*4:
        raise RuntimeError("missing or invalid 143h original GAMMA grid")
    import math, struct
    gamma_vals=struct.unpack("<357500f",gamma_raw.read_bytes())
    if not all(math.isfinite(v) and abs(v)<9999 for v in gamma_vals):
        raise RuntimeError("nonfinite or invalid original GAMMA")
    gamma_out=out/"original_gamma_143h.f32"
    shutil.copy2(gamma_raw,gamma_out)
    wind_raw=run_dir/"p6d_wind1_input_original.txt"
    if not wind_raw.is_file():
        raise RuntimeError("instrumented CALMET missing actual pre-WIND1 station columns")
    raw_rows=[]
    with wind_raw.open() as handle:
        for line in handle:
            v=line.split()
            if len(v)!=5:
                raise RuntimeError("bad pre-WIND1 station row")
            h,l,k=(int(x) for x in v[:3])
            if not (1<=h<=143 and 1<=l<=2 and 1<=k<=2):
                raise RuntimeError("invalid pre-WIND1 index")
            raw_rows.append((h,l,k,float(v[3].replace("D","E")),float(v[4].replace("D","E"))))
    if len(raw_rows)!=143*2*2 or len({x[:3] for x in raw_rows})!=143*2*2:
        raise RuntimeError("pre-WIND1 station trace incomplete")
    wind_csv=out/"original_wind1_station_inputs.csv"
    with wind_csv.open("w",newline="") as handle:
        w=csv.writer(handle)
        w.writerow(("hour","station","k","u","v"))
        w.writerows(raw_rows)
    upstream_trace=run_dir/"p6d_upstream_actual.txt"
    if not upstream_trace.exists():
        raise RuntimeError("instrumented CALMET missing pre/post-SLOPE snapshots")
    upstream_records=parse_upstream(upstream_trace)
    upstream_csv=out/"original_upstream_actual.csv"
    with upstream_csv.open("w",newline="") as handle:
        w=csv.DictWriter(handle,fieldnames=("label","hour_begin_packed","seconds",
                                          "kpt","i","j","k","u","v","qh","temp2d"))
        w.writeheader();w.writerows(upstream_records)
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
        "upstream_stage_rows": len(upstream_records),
        "upstream_csv_sha256": dgm.sha256_file(upstream_csv),
        "wind1_station_input_rows": len(raw_rows),
        "wind1_station_input_sha256": dgm.sha256_file(wind_csv),
        "original_inter2_pretail_bytes": len(raw_pretail),
        "original_inter2_pretail_sha256": dgm.sha256_file(pretail),
        "original_inter2_pretail_selected_sha256": pretail_artifacts,
        "original_inter2_pretail_stage": "after both DIAGNO INTER2 real invocations, before SMOOTH/DIVCEL; 143h original stream with six independently archived selected hours",
        "original_gamma_bytes": gamma_out.stat().st_size,
        "original_gamma_sha256": dgm.sha256_file(gamma_out),
        "original_gamma_layout": "143h x 50 x 50 float32 little-endian, i fastest",
        "upstream_scope": "original DIAGNO kpt7 immediately after WIND1, kpt8 before slope addition (also QH/TEMP2D), and kpt9 after slope addition; four cells two levels all 143 hours",
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
