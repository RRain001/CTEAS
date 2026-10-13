#!/usr/bin/env python3
"""Write-only AVETMP first-divergence trace (Fortran vs native C++), NOT a physics fix.

Instrument only temporary build copies, preserve accepted source and ensure the
instrumented original still generates the frozen 143h MET.DAT byte-for-byte.
The probe is at the 2022-01-01_19 TEMP2D worst cell: 1-based (12,19).
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "native_cpp/modules/p6_calmet/reference"))
import original_internal_state_dump as dgm

FOREIGN_ANCHOR = "      ztave(i,j)  = ztbar  / sumwt\n"
FOREIGN_PROBE = """      if (i.eq.12.and.j.eq.19) then
         open(93,file='dgm_avetmp_probe.txt',status='unknown',
     1        position='append')
         write(93,'(7(ES24.15E3,1X))') u(i,j),v(i,j),
     1        ztemp(i,j),sumwt,ztbar,ztave(i,j),eta
         close(93)
      endif
"""
CPP_ANCHOR = "            out[local]=ztbar/sumwt;\n"
CPP_PROBE = """            if(i==11 && j==18) {
                std::fprintf(stderr,
                    "CTEAS_AVETMP_PROBE %.9g %.9g %.9g %.9g %.9g %.9g %.9g\\n",
                    a.u_ms[local],a.v_ms[local],a.temperature_k[local],
                    sumwt,ztbar,out[local],eta);
            }
"""

def inject_once(content: str, anchor: str, extra: str) -> str:
    if content.count(anchor) != 1:
        raise RuntimeError("expected exactly one source-locked AVETMP anchor")
    return content.replace(anchor, anchor + extra, 1)

def original(args):
    audit = json.loads((args.audit_dir / "dgm-audit.json").read_text())
    if audit["status"] != "ORIGINAL_MET_DAT_IDENTICAL":
        raise RuntimeError("unverified original CALMET source")
    source = args.audit_dir / "calmet_instrumented.for"
    if dgm.sha256_file(source) != audit["instrumentation"]["instrumented_sha256"]["calmet_instrumented.for"]:
        raise RuntimeError("instrumented source changed")
    root = args.work / "original_avetmp_probe"
    root.mkdir(parents=True,exist_ok=True)
    source_copy = root / "calmet_avetmp_probe.for"
    source_copy.write_text(inject_once(source.read_text(), FOREIGN_ANCHOR, FOREIGN_PROBE))
    binary = root / "bin" / "calmet-avetmp-probe.x"
    dgm._build(args.compiler,source_copy,args.audit_dir / "include-alias",binary)
    case_dir = root / "case"
    dgm._stage_case(args.case.resolve(), case_dir)
    metdir = case_dir / "run" / "met"
    result = dgm._run(binary, metdir)
    if result["met_dat_sha256"] != dgm.TARGET_MET_DAT_SHA256:
        raise RuntimeError("AVETMP tracing perturbed original MET.DAT -- DO NOT TRUST")
    trace = metdir / "dgm_avetmp_probe.txt"
    if not trace.exists():raise RuntimeError("original AVETMP probe not created")
    (args.work/"original_avetmp_probe.txt").write_bytes(trace.read_bytes())
    print("ORIGINAL_AVETMP_WRITE_ONLY_FROZEN_MET_SHA_PASS",len(trace.read_text().splitlines()))

def cpp(args):
    header = args.header
    text = header.read_text()
    text = inject_once(text, CPP_ANCHOR, CPP_PROBE)
    text = text.replace("#include <cmath>", "#include <cmath>\n#include <cstdio>", 1)
    header.write_text(text)
    print("CXX_AVETMP_FORENSIC_BUILD_ONLY_PROBE_INSTALLED")

def report(args):
    originals=[]
    for line in (args.work/"original_avetmp_probe.txt").read_text().splitlines():
        values=[float(x.replace("D","E")) for x in line.split()]
        if len(values)!=7:raise ValueError("unexpected original AVETMP trace")
        originals.append(values)
    candidates=[]
    for line in (args.work/"cpp_avetmp_probe.log").read_text().splitlines():
        m=re.search(r"CTEAS_AVETMP_PROBE\s+(.+)$",line)
        if m:
            values=[float(x) for x in m.group(1).split()]
            if len(values)!=7:raise ValueError("unexpected native AVETMP trace")
            candidates.append(values)
    if len(candidates)!=143:raise RuntimeError(f"expected 143 native hourly probes, got {len(candidates)}")
    if len(originals)<143:raise RuntimeError(f"missing original traces: {len(originals)}")
    # A candidate must be compared to its original DIAGNO call, not a
    # possibly different COMP/AVETMP invocation at the same grid cell.
    # The first nearest ordering uses raw wind inputs and temperature as
    # discriminators; do not claim stage identity without verifying.
    results=[]
    for hour,cand in enumerate(candidates):
        distances=[(abs(ref[0]-cand[0])+abs(ref[1]-cand[1])+abs(ref[2]-cand[2]),idx)
                   for idx,ref in enumerate(originals)]
        dist,idx=min(distances)
        ref=originals[idx]
        results.append({"hour_index":hour,"candidate":cand,"original_nearest":ref,
                        "original_probe_index":idx,"input_L1_distance":dist,
                        "out_delta_K":cand[5]-ref[5],
                        "sumwt_delta":cand[3]-ref[3],
                        "ztbar_delta":cand[4]-ref[4]})
    report={"status":"FORENSIC_NOT_SCIENCE_PASS","grid_point":[12,19],
            "original_calls":len(originals),"native_calls":len(candidates),
            "sequence":results}
    outfile=args.work/"avetmp_first_divergence.json"
    outfile.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({"status":report["status"],"original_calls":len(originals),
          "native_calls":len(candidates),
          "worst_by_native_original_input_distance":
              sorted(results,key=lambda x:x["input_L1_distance"],reverse=True)[:5],
          "largest_output_delta":
              sorted(results,key=lambda x:abs(x["out_delta_K"]),reverse=True)[:5]},indent=2))

def main():
    p=argparse.ArgumentParser()
    p.add_argument("mode",choices=("original","cpp","report"))
    p.add_argument("--audit-dir",type=Path)
    p.add_argument("--case",type=Path)
    p.add_argument("--work",type=Path,required=True)
    p.add_argument("--header",type=Path)
    p.add_argument("--compiler",default="gfortran-11")
    a=p.parse_args()
    a.work.mkdir(parents=True,exist_ok=True)
    if a.mode=="original":original(a)
    elif a.mode=="cpp":cpp(a)
    else:report(a)

if __name__=="__main__":main()
