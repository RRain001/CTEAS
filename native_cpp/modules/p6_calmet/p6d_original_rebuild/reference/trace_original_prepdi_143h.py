#!/usr/bin/env python3
"""Source-pinned 143-hour CALMET PREPDI observed US/VS station-profile trace.

The original PREPDI call at calmet.for L4424-4426 emits its 2-station REAL*4
profiles to DIAGNO. A temporary write-only source copy captures those exact
profiles. Both untouched/instrumented Fortran must reproduce the SAME frozen
reference MET.DAT SHA256 before the trace may be used.

Mode 'compare' verifies the original post-PREPDI station profiles against
independently produced C++ WIND1 fixture inputs for every Hefei hour.
Tests the INPUT to WIND1 and INTER2, not just their numerical kernels.
"""
from __future__ import annotations
import argparse
import datetime as dt
import hashlib
import json
import math
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[5]
sys.path.insert(0,str(ROOT/"native_cpp/modules/p6_calmet/reference"))
import original_internal_state_dump as original

SOURCE_BLOB="437f43bb7f0e51a1919e56664663e1a046986ed0"
HEADER_FMT="<8i"
NLEVEL=9
RECORD_BYTES=32+4*4*NLEVEL

HELPER="""
c --- source-locked PREPDI US/VS trace, no state writeback
      subroutine dgmprepdi(us,vs,nxlev,nss,now,nu,yy,jul,hh,ss)
      include 'params.met'
      real us(mxnz,mxwnd),vs(mxnz,mxwnd)
      integer nxlev,nss,now,nu,yy,jul,hh,ss,k,n2
      logical opened
      save opened
      data opened/.false./
      n2=nss+now+1
      if(nss.ne.1.or.now.ne.0.or.nu.ne.1.or.nxlev.ne.9)stop 76
      if(.not.opened)then
          open(93,file='dgm_prepdi.bin',form='unformatted',
     1         access='stream',status='replace')
          opened=.true.
      endif
      write(93) yy,jul,hh,ss,nxlev,nss,now,nu
      write(93) (us(k,1),k=1,nxlev)
      write(93) (vs(k,1),k=1,nxlev)
      write(93) (us(k,n2),k=1,nxlev)
      write(93) (vs(k,n2),k=1,nxlev)
      return
      end
"""


def sha_file(f:Path) -> str:
    return hashlib.sha256(f.read_bytes()).hexdigest()


def trace(args):
    vendor=args.vendor.resolve();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    raw=(vendor/"calmet.for").read_bytes()
    oid=hashlib.sha1(f"blob {len(raw)}\0".encode()+raw).hexdigest()
    if oid!=SOURCE_BLOB or sha_file(vendor/"calmet.for")!=original.ORIGINAL_CALMET_SHA256:
        raise RuntimeError("original CALMET source blob drift")
    lines=raw.decode().splitlines(keepends=True)
    expected=[
        "         call prepdi(ws,wd,tempk,uaveaa,uavebb,vaveaa,vavebb,",
        "     1   nhre,nyrze,njulze,nhrze,nsecze,ziconv,",
        "     2   gamma,um,vm,wt,us,vs,nowsta,wsow,wdow)",
    ]
    if [x.rstrip("\r\n") for x in lines[4423:4426]]!=expected:
        raise RuntimeError("original PREPDI call source anchors changed")
    instrumented=out/"calmet_prepdi_trace.for"
    # Insert immediately AFTER original COMP PREPDI and BEFORE DIAGNO.
    injection=("c --- temporary source trace after unmodified PREPDI\n"
               "         call dgmprepdi(us,vs,nz,nssta,nowsta,nusta,\n"
               "     1                       nyrze,njulze,nhrze,nsecze)\n")
    instrumented.write_text("".join(lines[:4426])+injection+
                            "".join(lines[4426:])+HELPER)
    alias=original._alias_tree(vendor,out/"include_alias")
    bins=out/"bin";bins.mkdir(exist_ok=True)
    untouched=bins/"calmet_original"
    traced=bins/"calmet_prepdi_traced"
    command_untouched=original._build(args.compiler,vendor/"calmet.for",alias,untouched)
    command_traced=original._build(args.compiler,instrumented,alias,traced)
    staged=original._stage_case(args.case.resolve(),out/"case")
    run_dir=out/"case/run/met"
    data=run_dir/"dgm_prepdi.bin"
    if data.exists():data.unlink()
    a=original._run(untouched,run_dir)
    if data.exists():raise RuntimeError("untouched CALMET unexpectedly generated trace file")
    b=original._run(traced,run_dir)
    if (a["met_dat_sha256"]!=b["met_dat_sha256"] or
        b["met_dat_sha256"]!=original.TARGET_MET_DAT_SHA256):
        raise RuntimeError("PREPDI instrumentation changed reference MET.DAT")
    if not data.is_file():raise RuntimeError("original PREPDI source trace absent")
    records=read_records(data,args.hours)
    dump=out/"source_original_prepdi_143h.bin"
    shutil.copyfile(data,dump)
    proof={
        "status":"PASS_ORIGINAL_PREPDI_US_VS_REAL4_TRACE",
        "original_blob":oid,
        "original_fortran_sha256":original.ORIGINAL_CALMET_SHA256,
        "original_reference_met_dat_sha256":original.TARGET_MET_DAT_SHA256,
        "met_dat_byte_identical_to_untouched_and_frozen":True,
        "original_fortran_call":"COMP L4424–4426 PREPDI -> US/VS",
        "hours":len(records),
        "first_hour":records[0]["hour"],
        "last_hour":records[-1]["hour"],
        "float32_profile_record_bytes":RECORD_BYTES,
        "binary":dump.name,
        "binary_sha256":sha_file(dump),
        "data_source":"unaltered Fortran PREPDI outputs; station 1 surface, 2 upper",
        "scope":"source PREPDI observation handoff ONLY; does not prove full P6D wind",
    }
    (out/"original_prepdi_manifest.json").write_text(json.dumps(proof,indent=2)+"\n")
    print(json.dumps(proof,indent=2))


def read_records(path:Path,hours:int):
    raw=path.read_bytes()
    if len(raw)!=hours*RECORD_BYTES:
        raise RuntimeError(f"source PREPDI bytes {len(raw)} != {hours*RECORD_BYTES}")
    rows=[]
    for index in range(hours):
        base=index*RECORD_BYTES
        yy,jul,hh,ss,nz,nss,now,nu=struct.unpack_from(HEADER_FMT,raw,base)
        if (yy,nz,nss,now,nu)!=(2022,9,1,0,1) or not 0<=ss<3600:
            raise RuntimeError("original PREPDI record header mismatch")
        # PREPDI receives explicit UTC INTERVAL-END parameters
        # nyrze/njulze/nhrze/nsecze from COMP (calmet.for L4424-4426).
        # The trace must copy those exact arguments, never COMP's separate
        # nyre/njule/nhre clock. Do NOT reinterpret a wrong timestamp as
        # a one-hour start/end offset; compare actual hour-end UTC labels.
        source_end=dt.datetime(yy,1,1)+dt.timedelta(
            days=jul-1,hours=hh,seconds=ss)
        expected_end=dt.datetime(2022,1,1)+dt.timedelta(hours=index+1)
        if source_end!=expected_end:
            raise RuntimeError(
                f"PREPDI source UTC interval-end drift index={index+1}: "
                f"{source_end.isoformat()} vs {expected_end.isoformat()}")
        label=source_end.strftime("%Y-%m-%d_%H")
        values=struct.unpack_from("<36f",raw,base+32)
        if not all(math.isfinite(v) for v in values):
            raise RuntimeError("nonfinite PREPDI REAL*4")
        rows.append({"hour":label,"surface_u":values[:9],"surface_v":values[9:18],
                     "upper_u":values[18:27],"upper_v":values[27:36]})
    return rows


def compare(args):
    proof=json.loads(args.manifest.read_text())
    source=args.dump
    if (proof.get("status")!="PASS_ORIGINAL_PREPDI_US_VS_REAL4_TRACE" or
        not proof.get("met_dat_byte_identical_to_untouched_and_frozen") or
        proof.get("original_blob")!=SOURCE_BLOB or
        proof.get("original_reference_met_dat_sha256")!=original.TARGET_MET_DAT_SHA256 or
        sha_file(source)!=proof.get("binary_sha256")):
        raise RuntimeError("PREPDI Fortran trace chain of custody failed")
    rows=read_records(source,143)
    fields=args.fixture_dir
    maxima={"surface_u":0.0,"surface_v":0.0,"upper_u":0.0,"upper_v":0.0}
    bad=[];checked=0
    # Original CALMET PREPDI uses EDIT=999.f. In the selected IEXTRP=-4
    # branch, upper-air level 1 is excluded. C++ WIND1's fixture maps the
    # missing datum to its own internal 999999.f sentinel. Require this exact
    # restricted equivalence; never treat a missing wind as real 999 m/s.
    original_missing=999.0
    cpp_missing=999999.0
    excluded_level_one=0
    missing_cutoff=999998.0
    tol=2e-4
    for idx,row in enumerate(rows):
        f=fields/f"wind1_driver_{row['hour']}.inp"
        if not f.is_file():raise RuntimeError("missing real P6D fixture "+str(f))
        lines=f.read_text().splitlines()
        if len(lines)!=11:
            raise RuntimeError("P6D fixture unexpected number of rows")
        first=lines[0].split()
        if len(first)!=10:raise RuntimeError("station fixture header length wrong")
        observed={"surface_u":float(first[4]),"surface_v":float(first[5])}
        for k in range(9):
            vals=lines[k+2].split()
            if len(vals)!=2:raise RuntimeError("station upper fixture row wrong")
            observed[f"upper_u_{k}"]=float(vals[0])
            observed[f"upper_v_{k}"]=float(vals[1])
        for key in ("surface_u","surface_v"):
            expected=float(row[key][0]);modern=observed[key]
            if abs(expected)>missing_cutoff or abs(modern)>missing_cutoff:
                bad.append({"hour":row["hour"],"component":key,"reason":"missing source wind"})
                continue
            difference=abs(expected-modern)
            maxima[key]=max(maxima[key],difference);checked+=1
            if difference>tol and len(bad)<30:
                bad.append({"hour":row["hour"],"component":key,
                            "original":expected,"modern":modern,"error":difference})
        for key in ("upper_u","upper_v"):
            for k in range(9):
                expected=float(row[key][k]);modern=observed[f"{key}_{k}"]
                if k==0:
                    if expected==original_missing and modern==cpp_missing:
                        excluded_level_one+=1
                        continue
                    if len(bad)<30:
                        bad.append({"hour":row["hour"],"level":1,"component":key,
                                    "original":expected,"modern":modern,
                                    "reason":"invalid IEXTRP<=0 first-level sentinel"})
                    continue
                if (abs(expected)>=original_missing or
                    abs(modern)>=missing_cutoff):
                    if len(bad)<30:bad.append({
                        "hour":row["hour"],"level":k+1,"component":key,
                        "original":expected,"modern":modern,
                        "reason":"unexpected upper-layer missing marker"})
                    continue
                difference=abs(expected-modern)
                maxima[key]=max(maxima[key],difference);checked+=1
                if difference>tol and len(bad)<30:
                    bad.append({"hour":row["hour"],"level":k+1,"component":key,
                                "original":expected,"modern":modern,"error":difference})
    if excluded_level_one!=len(rows)*2:
        raise RuntimeError("upper-level-one missing-count contract incomplete")
    report={
        "first_upper_layer_missing_semantics":{
            "original_PREPDI_EDIT":original_missing,
            "cpp_WIND1_fixture_sentinel":cpp_missing,
            "IEXTRP_case":-4,
            "excluded_UV_values":excluded_level_one,
            "exactly_scoped_to_upper_level_1":True},
        "status":"PASS_ORIGINAL_PREPDI_VS_CPP_INPUT" if not bad else "FAIL_PREPDI_INPUT_PARITY",
        "scoped_original_Fortran_PREPDI_input_real4":True,
        "original_met_dat_unmodified":True,
        "max_abs_error_mps":maxima,
        "float_values_compared":checked,
        "hours":len(rows),"comparison_tolerance_mps":tol,
        "first_mismatches":bad,
        "scope":"post-PREPDI REAL4 station winds vs C++ fixture; NOT complete DIAGNO",
    }
    args.out.mkdir(parents=True,exist_ok=True)
    (args.out/"prepdi_143h_original_vs_cpp.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))
    if bad:raise SystemExit(1)


def main():
    p=argparse.ArgumentParser()
    p.add_argument("command",choices=["trace","compare"])
    p.add_argument("--vendor",type=Path)
    p.add_argument("--case",type=Path)
    p.add_argument("--out",required=True,type=Path)
    p.add_argument("--compiler",default="gfortran-11")
    p.add_argument("--hours",type=int,default=143)
    p.add_argument("--manifest",type=Path)
    p.add_argument("--dump",type=Path)
    p.add_argument("--fixture-dir",type=Path)
    args=p.parse_args()
    if args.command=="trace":
        if not args.vendor or not args.case:raise SystemExit("trace requires vendor/case")
        trace(args)
    else:
        if not args.manifest or not args.dump or not args.fixture_dir:
            raise SystemExit("compare requires manifest/dump/fixture-dir")
        compare(args)


if __name__=="__main__":
    main()
