#!/usr/bin/env python3
"""Forensic 143-hour original CALMET INTER2 layer-1 first-divergence gate.

The original vendor calmet.for is never edited: a SHA-pinned write-only copy
takes surface/upper US/VS(1,:) just before INTER2 output, and U/V level one
immediately after both INTER2 calls. Original MET.DAT must remain bitwise
identical to the frozen 2022 reference. Native P6D station and INTER2 fields
are independent. This is DIAGNOSTIC, not complete native P6D acceptance.
"""
from __future__ import annotations
import argparse
import csv
import datetime as dt
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"native_cpp/modules/p6_calmet/reference"))
import original_internal_state_dump as vendor_audit

NX=NY=50
NH=143
PLANE=2500
HEADER="<5i4f"
H_SIZE=struct.calcsize(HEADER)
REC_SIZE=H_SIZE+2*PLANE*4
# This helper only READS original COMMON state, then writes its own file;
# it does not modify the original arrays, calls, or output stream.
HELPER=r"""
c --- P6D source-locked write-only layer-1 INTER2 chain trace
      subroutine dgmfirstlayer1
      include 'params.met'
      include 'gen.met'
      include 'grid.met'
      include 'd1.met'
      integer i,j
      logical opened
      save opened
      data opened/.false./
      if(.not.opened)then
         open(94,file='dgm_inter2_layer1.bin',
     1        form='unformatted',access='stream',status='replace')
         opened=.true.
      endif
      write(94) 12,ndathrb,nsecb,nx,ny
      write(94) us(1,1),vs(1,1),us(1,2),vs(1,2)
      write(94) ((u(i,j,1),i=1,nx),j=1,ny)
      write(94) ((v(i,j,1),i=1,nx),j=1,ny)
      return
      end
"""
STATION_COLS=("surface_u","surface_v","upper_u","upper_v")
TOL={
    "surface_u":(2.e-5,1.e-5),"surface_v":(2.e-5,1.e-5),
    "upper_u":(2.e-5,1.e-5),"upper_v":(2.e-5,1.e-5),
    "inter2_u":(1.e-4,1.e-5),"inter2_v":(1.e-4,1.e-5)
}
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def label(hour):
    return (dt.datetime(2022,1,1)+dt.timedelta(hours=hour)).strftime("%Y-%m-%d_%H")
def patch_vendor(original,target):
    if digest(original)!=vendor_audit.ORIGINAL_CALMET_SHA256:
        raise RuntimeError("unaltered original calmet.for provenance drift")
    lines=original.read_text().splitlines(keepends=True)
    if (lines[6054].strip()!="CALL INTER2(US,NWIND,UTMXST,UTMYST," or
        lines[6058].strip()!="CALL INTER2(VS,NWIND,UTMXST,UTMYST," or
        lines[6071].strip()!="ENDIF" or
        "Lake breeze region" not in lines[6075]):
        raise RuntimeError("original INTER2 call-order insertion anchors changed")
    lines.insert(6072,"      call dgmfirstlayer1\n")
    target.write_text("".join(lines)+HELPER)
def read_orig(path):
    raw=path.read_bytes()
    if len(raw)!=NH*REC_SIZE:
        raise RuntimeError(f"original layer-1 record count/shape mismatch: {len(raw)}")
    for h in range(NH):
        begin=h*REC_SIZE
        kind,packed,second,nx,ny,*station=struct.unpack_from(HEADER,raw,begin)
        time=dt.datetime(2022,1,1)+dt.timedelta(hours=h)
        stamp=time.year*100000+time.timetuple().tm_yday*100+time.hour
        if (kind,packed,second,nx,ny)!=(12,stamp,0,NX,NY):
            raise RuntimeError(f"source INTER2 sequence/size/hour mismatch {h+1}")
        u=struct.unpack_from("<2500f",raw,begin+H_SIZE)
        v=struct.unpack_from("<2500f",raw,begin+H_SIZE+PLANE*4)
        yield dict(zip(STATION_COLS,station))|{"inter2_u":u,"inter2_v":v}
def read_native_grid(path):
    raw=path.read_bytes()
    if len(raw)!=9*PLANE*4:
        raise RuntimeError(f"native INTER2 3D field shape drift: {path}")
    return struct.unpack_from("<2500f",raw)
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--vendor",type=Path,required=True)
    ap.add_argument("--case",type=Path,required=True)
    ap.add_argument("--candidate",type=Path,required=True)
    ap.add_argument("--out",type=Path,required=True)
    ap.add_argument("--compiler",default="gfortran-11")
    ap.add_argument("--require-parity",action="store_true")
    a=ap.parse_args()
    vendor=a.vendor.resolve();case=a.case.resolve();native=a.candidate.resolve()
    if digest(vendor/"calutils.for")!=vendor_audit.ORIGINAL_CALUTILS_SHA256:
        raise RuntimeError("original CALMET calutils.for checksum drift")
    work=a.out.resolve().parent/"firstlayer-fortran"
    work.mkdir(parents=True,exist_ok=True)
    trace_source=work/"calmet_firstlayer.for"
    patch_vendor(vendor/"calmet.for",trace_source)
    alias=vendor_audit._alias_tree(vendor,work/"includes")
    exe=work/"bin"/"met-original-with-firstlayer-writeonly.x"
    vendor_audit._build(a.compiler,trace_source,alias,exe)
    staged=work/"case"
    if staged.exists():
        shutil.rmtree(staged)
    vendor_audit._stage_case(case,staged)
    run_dir=staged/"run/met"
    result=vendor_audit._run(exe,run_dir)
    if result["met_dat_sha256"]!=vendor_audit.TARGET_MET_DAT_SHA256:
        raise RuntimeError("write-only original instrumentation changed frozen MET.DAT bytes")
    source=run_dir/"dgm_inter2_layer1.bin"
    if not source.is_file():
        raise RuntimeError("original INTER2 first-layer trace absent")
    with (native/"station_inter2_layer1.csv").open(newline="") as f:
        rows=list(csv.DictReader(f))
    if len(rows)!=NH or list(rows[0])!=["hour_end",*STATION_COLS]:
        raise RuntimeError("native first-layer station input stream mismatch")
    stats={k:{"compared":0,"exceedances":0,"max_abs":0.,
              "first_exceedance":None,"worst":None,"hour_exceedances":{}}
           for k in TOL}
    for h,orig in enumerate(read_orig(source),1):
        actual_row=rows[h-1]
        if actual_row["hour_end"]!=label(h):
            raise RuntimeError("native station INTER2 hour-end label drift")
        for field in TOL:
            reference=orig[field]
            got=(float(actual_row[field]) if field in STATION_COLS else
                 read_native_grid(native/f"{field}_{label(h)}.f32"))
            rr=(reference,) if field in STATION_COLS else reference
            nn=(got,) if field in STATION_COLS else got
            abstol,reltol=TOL[field]
            st=stats[field]
            for n,(ref,val) in enumerate(zip(rr,nn)):
                if not(math.isfinite(ref) and math.isfinite(val)):
                    raise RuntimeError(f"nonfinite source/candidate {field} hour {h}")
                delta=abs(ref-val)
                st["compared"]+=1
                if delta>st["max_abs"]:
                    st["max_abs"]=delta
                    st["worst"]={"hour":h,"i":n%NX+1,"j":n//NX+1,
                                 "original":ref,"native":val,"abs":delta}
                if delta>abstol+reltol*abs(ref):
                    st["exceedances"]+=1
                    st["hour_exceedances"][str(h)]=st["hour_exceedances"].get(str(h),0)+1
                    if st["first_exceedance"] is None:
                        st["first_exceedance"]={"hour":h,"i":n%NX+1,"j":n//NX+1,
                                                "original":ref,"native":val,"abs":delta}
    for field,st in stats.items():
        if st["compared"]!=(NH if field in STATION_COLS else NH*PLANE):
            raise RuntimeError(f"incomplete native original INTER2 field {field}")
    failed=[k for k,v in stats.items() if v["exceedances"]]
    report={
        "status":"PASS_FIRST_LAYER_INTER2_SOURCE_143H" if not failed else
                 "FAIL_FIRST_LAYER_INTER2_SOURCE_143H",
        "first_failing_fields":failed,
        "first_failing_hour":min((v["first_exceedance"]["hour"] for v in stats.values()
                                  if v["first_exceedance"]),default=None),
        "source_instrumented_sha256":digest(trace_source),
        "source_original_sha256":vendor_audit.ORIGINAL_CALMET_SHA256,
        "frozen_met_dat_sha256":result["met_dat_sha256"],
        "trace_file_sha256":digest(source),
        "no_original_intermediate_injected_into_native":True,
        "levels_compared":[1],"hours":NH,"grid":[NX,NY],
        "stage":"DIAGNO source 6072 after INTER2 U/V and station US/VS level1",
        "fields":stats,"tolerances":TOL,
        "full_143h_UVW_pass":False}
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({"status":report["status"],
                      "first_failing_hour":report["first_failing_hour"],
                      "summary":{k:{"max_abs":v["max_abs"],"exceedances":v["exceedances"],
                                     "first":v["first_exceedance"],"worst":v["worst"],
                                     "hour_exceedances":v["hour_exceedances"]}
                                 for k,v in stats.items()}},indent=2))
    if a.require_parity and failed:
        raise SystemExit(1)
if __name__=="__main__":
    main()
