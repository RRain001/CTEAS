#!/usr/bin/env python3
"""Write-only original CALPUFF H-3 CALCPF intermediate trace for Hefei hour 1.

The original vendor source is SHA pinned, untouched, compiled from a temporary
instrumented copy. Frozen puff.con and 143-hour COMP DGM hashes must match.
Only first-hour grid receptor (ISAMP=19, JSAMP=50), species IPOL=1 are traced.
A diagnostic trace is an oracle of original data, NOT a native parity claim.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import original_puff_step_dump as original

DGM_SHA = "cbbdaa29c6222e6ca6aefe9f605ad13e7dd6d258f6ad243afcac07cb578108c6"
RECORD = struct.Struct("<3i13f")
# This is the exact original CALCPF L4304 statement, checked byte-for-byte.
CALCPF_ANCHOR = "                  conc=vdpvd(ipol)*xtemp*(qold(ipol)*t1+qnew(ipol)*t2)"
CALL = """                  call dgmrecv(isamp,jsamp,ipol,syrec,szrec,zpr,
     &                 zrpole,f1,xi1,xi2,t1,t2,qold(ipol),
     &                 qnew(ipol),xtemp,conc)
                  call dgmrecvvirt(isamp,jsamp,ipol,frec,
     &                 vtyb1,vtye1,vdyb1,vdye1,
     &                 vtzb1,vtze1,vdzb1,vdze1)
"""

# Helpers are appended AFTER all original modules; no COMMON values are
# mutated except the new isolated /DGMRECUR/ diagnostic counter.
EXTRA_HELPERS = """
c ---- Write-only receiver-stage provenance, original first hour only.
      subroutine dgmreceiverstep(ii,ksam)
      integer ii,ksam,step_count,dgmseq,dgmid,dgmksam
      common /dgmrecur/ dgmseq,dgmid,dgmksam
      save step_count
      data step_count/0/
      step_count=step_count+1
      dgmseq=step_count
      dgmid=ii
      dgmksam=ksam
      return
      end
      subroutine dgmrecv(isamp,jsamp,ipol,syrec,szrec,zpr,
     &                   zrpole,f1,xi1,xi2,t1,t2,qold,qnew,
     &                   xtemp,conc)
      integer isamp,jsamp,ipol,dgmseq,dgmid,dgmksam
      real syrec,szrec,zpr,zrpole,f1,xi1,xi2,t1,t2
      real qold,qnew,xtemp,conc
      common /dgmrecur/ dgmseq,dgmid,dgmksam
      logical first
      save first
      data first/.true./
      if(dgmseq.gt.1275) return
      if(isamp.ne.19.or.jsamp.ne.50.or.ipol.ne.1) return
      if(first)then
        open(unit=97,file='dgm_calcpf_h3_19_50.bin',
     &       form='unformatted',access='stream',
     &       status='replace')
        first=.false.
      endif
      write(97) dgmseq,dgmid,dgmksam,
     &          syrec,szrec,zpr,zrpole,f1,xi1,xi2,t1,t2,
     &          qold,qnew,xtemp,conc
      return
      end
      subroutine dgmrecvvirt(isamp,jsamp,ipol,frac,
     &                       vtyb,vtye,vdyb,vdye,
     &                       vtzb,vtze,vdzb,vdze)
      integer isamp,jsamp,ipol,dgmseq,dgmid,dgmksam
      real frac,vtyb,vtye,vdyb,vdye,vtzb,vtze,vdzb,vdze
      common /dgmrecur/ dgmseq,dgmid,dgmksam
      logical first
      save first
      data first/.true./
      if(dgmseq.gt.1275) return
      if(isamp.ne.19.or.jsamp.ne.50.or.ipol.ne.1) return
      if(first)then
        open(unit=98,file='dgm_calcpf_h3_virtual_19_50.bin',
     &       form='unformatted',access='stream',
     &       status='replace')
        first=.false.
      endif
      write(98) dgmseq,dgmid,dgmksam,
     &          frac,vtyb,vtye,vdyb,vdye,vtzb,vtze,vdzb,vdze
      return
      end
"""
FIELD_NAMES = ("syrec", "szrec", "zpr", "zrpole", "f1", "xi1", "xi2",
               "t1", "t2", "qold", "qnew", "xtemp", "conc")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--vendor", type=Path, required=True)
    ap.add_argument("--case", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--compiler", default="gfortran-11")
    args = ap.parse_args()
    vendor, case, out = (p.resolve() for p in (args.vendor, args.case, args.out))
    # The base extraction records original source assertions at each COMP
    # insertion and independently pins both original Fortran source hashes.
    original.CALLS["item7_setpuf_return"] += (
        "            call dgmreceiverstep(ii,ksam)\n"
    )
    original.instrument(vendor, out)
    instrumented = out / "calpuff_instrumented.for"
    source = instrumented.read_text(encoding="latin-1")
    if source.count(CALCPF_ANCHOR) != 1:
        raise AssertionError("original CALCPF mass-weighting anchor changed")
    source = source.replace(CALCPF_ANCHOR + "\n",
                            CALCPF_ANCHOR + "\n" + CALL)
    instrumented.write_text(source + EXTRA_HELPERS, encoding="latin-1")

    alias = original._alias_tree(vendor, out / "include-alias")
    executable = out / "bin" / "puff-h3-receiver.x"
    original._build(args.compiler, vendor / "modules.for", instrumented,
                    alias, executable)
    staged = out / "case"
    original._stage_case(case, staged)
    run = staged / "run" / "puff"
    result = original._run(executable, run)
    if result["puff_con_sha256"] != original.TARGET_PUFF_CON_SHA256:
        raise AssertionError(
            "original CALPUFF receiver instrumentation changed puff.con: "
            + result["puff_con_sha256"])
    dgm = run / "dgm_puffstep.bin"
    if original.sha256_file(dgm) != DGM_SHA or dgm.stat().st_size != 254687 * 352:
        raise AssertionError("original 143h DGM changed under CALCPF instrumentation")
    trace = run / "dgm_calcpf_h3_19_50.bin"
    if not trace.is_file() or trace.stat().st_size % RECORD.size:
        raise AssertionError("missing or misaligned original CALCPF H3 trace")
    samples = []
    with trace.open("rb") as fd:
        while chunk := fd.read(RECORD.size):
            if len(chunk) != RECORD.size:
                raise AssertionError("truncated H-3 receptor record")
            seq, puff, ksam, *values = RECORD.unpack(chunk)
            if not (1 <= seq <= 1275 and 1 <= puff <= 54 and
                    1 <= ksam <= 54 and all(map(lambda x: abs(x)<1e100, values))):
                raise AssertionError("original CALCPF key or value corrupted")
            samples.append((seq, puff, ksam, values))
    if not samples:
        raise AssertionError("original first-hour H3 hotspot never sampled")
    virtual_trace = run / "dgm_calcpf_h3_virtual_19_50.bin"
    if not virtual_trace.is_file() or virtual_trace.stat().st_size != len(samples) * 48:
        raise AssertionError("original first-hour PUFRECS virtual state alignment mismatch")
    with virtual_trace.open("rb") as infile:
        for k, original_row in enumerate(samples):
            row = infile.read(48)
            vseq, vid, vksam, *stage = struct.unpack("<3i9f", row)
            if (vseq, vid, vksam) != original_row[:3]:
                raise AssertionError(f"PUFRECS virtual trace identity drift at row {k}")
            if k < 8:
                print("P7_VENDOR_H3_PUFRECS_VIRTUAL"
                      f" SEQ={vseq} II={vid} KSAM={vksam}"
                      + " ".join(f" {name}={value:.9g}" for name, value in zip(
                          ("frac", "vtyb", "vtye", "vdyb", "vdye",
                           "vtzb", "vtze", "vdzb", "vdze"),stage)))
    if any(a[0] >= b[0] for a, b in zip(samples, samples[1:])):
        raise AssertionError("original receiver substep trace order incorrect")
    total = sum(v[12] for _, _, _, v in samples)
    audit = {
        "scope": "original CALPUFF first-hour H3 receptor (19,50), source-only",
        "vendor_sha256": original.ORIGINAL_CALPUFF_SHA256,
        "frozen_puff_con_sha256": result["puff_con_sha256"],
        "frozen_dgm_sha256": original.sha256_file(dgm),
        "records": len(samples),
        "record_bytes": RECORD.size,
        "trace_sha256": original.sha256_file(trace),
        "trace_path": str(trace),
        "virtual_trace_path": str(virtual_trace),
        "virtual_trace_sha256": original.sha256_file(virtual_trace),
        "first_receptor_contribution": {
            "seq": samples[0][0], "puff": samples[0][1],
            "ksam": samples[0][2],
            **dict(zip(FIELD_NAMES, samples[0][3])),
        },
        "trace_contribution_sum": total,
        "numeric_native_parity": "NOT_CLAIMED",
    }
    (out / "original-calcpf-h3-19-50.json").write_text(
        json.dumps(audit, indent=2) + "\n")
    print("P7_ORIGINAL_H3_CALCPF_RECEIVER_ORACLE=PASS"
          f" SAMPLES={len(samples)}"
          f" FIRST_STEP={samples[0][0]}"
          f" SOURCE_CONC_SUM={total:.9g}"
          " CONCENTRATION_REFERENCE_UNCHANGED=YES"
          " FULL_DGM_REFERENCE_UNCHANGED=YES"
          " NATIVE_PUFRECS_STAGE_PARITY=OPEN")
    for seq, puff, ksam, values in samples[:12]:
        print(f"P7_VENDOR_H3_CALCPF_FIRST_STEPS"
              f" SEQ={seq} II={puff} KSAM={ksam} "
              + " ".join(f"{name}={value:.9g}"
                         for name, value in zip(FIELD_NAMES, values)))


if __name__ == "__main__":
    main()
