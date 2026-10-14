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
import math
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
                  call dgmrecvfrac(isamp,jsamp,ipol,frec)
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
      subroutine dgmrecvfrac(isamp,jsamp,ipol,frac)
      integer isamp,jsamp,ipol,dgmseq,dgmid,dgmksam
      real frac
      common /dgmrecur/ dgmseq,dgmid,dgmksam
      if(dgmseq.gt.1275) return
      if(isamp.ne.19.or.jsamp.ne.50.or.ipol.ne.1) return
      write(97) dgmseq,dgmid,dgmksam,frac
      return
      end
"""
FIELD_NAMES = ("syrec", "szrec", "zpr", "zrpole", "f1", "xi1", "xi2",
               "t1", "t2", "qold", "qnew", "xtemp", "conc")

# Seven exact, source-ordered SETPUF checkpoints at the first real H9 calm
# vertical-sigma mismatch (global DGM #16426, puff 588). These are inserted
# only in a throwaway compiled vendor copy. Binary output is read-only, and
# the complete unmodified original puff.con/DGM hashes MUST remain exact.
H9_Z_TRACE_NAME = "dgm_h9_puff588_setpuf_z.bin"
H9_Z_RECORD = struct.Struct("<3i8f")
H9_Z_HELPERS = """
c --- Write-only exact H9 vertical SETPUF source state, no state mutation
      subroutine dgmh9z(ii,phase,szb,szm,sze,vzb,vzm,vze,tm,te)
      include 'params.puf'
      include 'datehr.puf'
      integer ii,phase
      real szb,szm,sze,vzb,vzm,vze,tm,te
      logical ldgm
      save ldgm
      data ldgm/.false./
      if(ii.ne.588.or.ndathrb.ne.202200108) return
      if(.not.ldgm)then
         open(98,file='dgm_h9_puff588_setpuf_z.bin',
     &        access='stream',form='unformatted',status='replace')
         ldgm=.true.
      endif
      write(98) phase,ii,ndathrb,szb,szm,sze,vzb,vzm,vze,tm,te
      return
      end
"""
H9_Z_ANCHORS = (
    # Original SETPUF calpuff.for L48526-48534, BEFORE terrain or clipping.
    ("            call sigtz(sigzb(ii),zero,zero,zpb(ii),szb1,vtzb1,vdzb1)",
     1, "szb1,szm1,sze1,vtzb1,zero,zero,zero,zero"),
    ("            call sigtz(zero,dm,tm,zpb(ii),szm1,vtzm1,vdzm1)",
     2, "szb1,szm1,sze1,vtzb1,vtzm1,zero,tm,zero"),
    ("            call sigtz(zero,de,te,zpb(ii),sze1,vtze1,vdze1)",
     3, "szb1,szm1,sze1,vtzb1,vtzm1,vtze1,tm,te"),
    # Original SETPUF CTADJ2 state, after inverse-time correction.
    ("            call sigtz(szadj,zero,zero,zpb(ii),szm1,vtzm1,vdzm1)",
     4, "szb1,szm1,sze1,vtzb1,vtzm1,vtze1,tm,te"),
    ("            call sigtz(szadj,zero,zero,zpb(ii),sze1,vtze1,vdze1)",
     5, "szb1,szm1,sze1,vtzb1,vtzm1,vtze1,tm,te"),
    # Original SETPUF L48635-48639 monotonicity/stored sigma transition.
    ("         if(szm1.GT.sze1) sze1=szm1",
     6, "szb1,szm1,sze1,vtzb1,vtzm1,vtze1,tm,te"),
    ("         sigzb(ii)=sze1",
     7, "szb1,szm1,sze1,vtzb1,vtzm1,vtze1,tm,te"),
)

def add_h9_setpuf_trace(source: str) -> str:
    # Verify every exact Fortran statement once; abort instead of installing
    # the diagnostic in an unrelated SIGTZ call.
    for anchor, phase, fields in H9_Z_ANCHORS:
        if source.count(anchor + "\n") != 1:
            raise AssertionError(
                f"source SETPUF H9 trace anchor drift phase={phase} {anchor!r}")
        parts=fields.split(",")
        statement=(
            f"            call dgmh9z(ii,{phase},"
            + ",".join(parts[:4]) + ",\n"
            + "     &                " + ",".join(parts[4:]) + ")\n"
        )
        source=source.replace(anchor + "\n",anchor + "\n" + statement)
    return source + H9_Z_HELPERS

def audit_h9_setpuf_trace(path: Path, out: Path) -> None:
    if not path.is_file() or path.stat().st_size % H9_Z_RECORD.size:
        raise AssertionError("original H9 SETPUF trace missing/misaligned")
    rows=[]
    with path.open("rb") as handle:
        while raw:=handle.read(H9_Z_RECORD.size):
            phase,ii,packed,*values=H9_Z_RECORD.unpack(raw)
            if ii!=588 or packed!=202200108 or phase not in range(1,8):
                raise AssertionError("invalid source H9 SETPUF trace identity")
            rows.append((phase,values))
    if len(rows)<7 or [r[0] for r in rows[:7]]!=list(range(1,8)):
        raise AssertionError("source H9 puff588 phase order not complete")
    labels=("SZB","SZM","SZE","VTZB","VTZM","VTZE","TM","TE")
    def r4bits(x: float) -> str:
        return f"0x{struct.unpack('<I',struct.pack('<f',x))[0]:08x}"
    for phase,values in rows[:7]:
        print(
            f"P7_ORIGINAL_H9_SETPUF_Z_PHASE={phase} PUFF=588 "
            + " ".join(f"{label}={value:.10g} BIT_{label}={r4bits(value)}"
                       for label,value in zip(labels,values)))
    out.write_text(json.dumps({
        "status": "ORIGINAL_H9_SOURCE_ONLY", "phase_order": list(range(1,8)),
        "record_count": len(rows),
        "record_bytes": H9_Z_RECORD.size,
        "binary_sha256": original.sha256_file(path),
        "first_complete_step": [
            {"phase":phase, **{k:v for k,v in zip(labels,values)}}
            for phase,values in rows[:7]
        ],
        "native_state_injected": False,
        "vendor_source_unchanged": True
    },indent=2)+"\n")
    print(f"P7_ORIGINAL_H9_SETPUF_Z_TRACE=PASS ROWS={len(rows)}"
          " STAGES=1,2,3,4,5,6,7"
          " ORIGINAL_PUFF_CON_AND_DGM=UNCHANGED"
          " NATIVE_H9_PARITY=OPEN")



def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--vendor", type=Path, required=True)
    ap.add_argument("--case", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--compiler", default="gfortran-11")
    ap.add_argument("--hour", type=int, choices=(1, 2, 4), default=1)
    ap.add_argument("--receptor-i", type=int, default=None)
    ap.add_argument("--receptor-j", type=int, default=None)
    ap.add_argument("--trace-h9-setpuf", action="store_true")
    args = ap.parse_args()
    vendor, case, out = (p.resolve() for p in (args.vendor, args.case, args.out))
    defaults = {1:(19,50), 2:(25,28), 4:(47,13)}
    di,dj=defaults[args.hour]
    ix = args.receptor_i if args.receptor_i is not None else di
    jy = args.receptor_j if args.receptor_j is not None else dj
    if not (1 <= ix <= 50 and 1 <= jy <= 50):
        raise ValueError("Hefei original receptor coordinates outside frozen grid")
    first_seq, final_seq = {1:(1,1275),2:(1276,3517),4:(5424,7555)}[args.hour]
    last_puff = {1:54,2:137,4:272}[args.hour]
    trace_name = (f"dgm_calcpf_h3_{ix}_{jy}.bin" if args.hour == 1 else
                  f"dgm_calcpf_h3_{ix}_{jy}_hour{args.hour}.bin")
    source_helpers = (EXTRA_HELPERS
        .replace("if(dgmseq.gt.1275) return",
                 f"if(dgmseq.lt.{first_seq}.or.dgmseq.gt.{final_seq}) return")
        .replace("if(isamp.ne.19.or.jsamp.ne.50.or.ipol.ne.1) return",
                 f"if(isamp.ne.{ix}.or.jsamp.ne.{jy}.or.ipol.ne.1) return")
        .replace("dgm_calcpf_h3_19_50.bin",trace_name))
    if source_helpers.count(f"if(dgmseq.lt.{first_seq}.or.dgmseq.gt.{final_seq}) return") != 2 or \
       source_helpers.count(f"if(isamp.ne.{ix}.or.jsamp.ne.{jy}.or.ipol.ne.1) return") != 2:
        raise AssertionError("original receiver instrumentation guards not exact")
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
    if args.trace_h9_setpuf:
        source=add_h9_setpuf_trace(source)
    instrumented.write_text(source + source_helpers, encoding="latin-1")

    alias = original._alias_tree(vendor, out / "include-alias")
    executable = out / "bin" / "puff-h3-receiver.x"
    original._build(args.compiler, vendor / "modules.for", instrumented,
                    alias, executable)
    staged = out / "case"
    original._stage_case(case, staged)
    run = staged / "run" / "puff"
    try:
        result = original._run(executable, run)
    except SystemExit:
        vendor_log = run / "puff_runtime.log"
        if vendor_log.exists():
            print("P7_ORIGINAL_CALCPF_INSTRUMENTED_VENDOR_RUNTIME_TAIL_BEGIN",
                  file=sys.stderr)
            print(vendor_log.read_text(errors="replace")[-8000:],
                  file=sys.stderr)
            print("P7_ORIGINAL_CALCPF_INSTRUMENTED_VENDOR_RUNTIME_TAIL_END",
                  file=sys.stderr)
        raise
    if result["puff_con_sha256"] != original.TARGET_PUFF_CON_SHA256:
        raise AssertionError(
            "original CALPUFF receiver instrumentation changed puff.con: "
            + result["puff_con_sha256"])
    dgm = run / "dgm_puffstep.bin"
    if original.sha256_file(dgm) != DGM_SHA or dgm.stat().st_size != 254687 * 352:
        raise AssertionError("original 143h DGM changed under CALCPF instrumentation")
    if args.trace_h9_setpuf:
        audit_h9_setpuf_trace(run / H9_Z_TRACE_NAME,
                             out / "original-h9-puff588-setpuf-z.json")
    trace = run / trace_name
    if not trace.is_file() or trace.stat().st_size % (RECORD.size+16):
        raise AssertionError("missing or misaligned original CALCPF H3 trace")
    samples = []
    with trace.open("rb") as fd:
        while chunk := fd.read(RECORD.size):
            if len(chunk) != RECORD.size:
                raise AssertionError("truncated H-3 receptor record")
            fraction_block = fd.read(16)
            if len(fraction_block) != 16:
                raise AssertionError("missing paired PUFRECS fraction record")
            seq, puff, ksam, *values = RECORD.unpack(chunk)
            vseq, vid, vksam, frec = struct.unpack("<3if",fraction_block)
            if (seq,puff,ksam)!=(vseq,vid,vksam):
                raise AssertionError("original CALCPF stage/virtual identity mismatch")
            if not (first_seq <= seq <= final_seq and 1 <= puff <= last_puff and
                    1 <= ksam <= 99 and all(math.isfinite(x) for x in values+[frec])):
                raise AssertionError("original CALCPF key or value corrupted")
            samples.append((seq, puff, ksam, values))
            if len(samples) <= 8:
                print(f"P7_VENDOR_H3_PUFRECS_FRAC SEQ={vseq}"
                      f" II={vid} KSAM={vksam} FRAC={frec:.9g}")
    if not samples:
        raise AssertionError("original first-hour H3 hotspot never sampled")
    if any(a[0] >= b[0] for a, b in zip(samples, samples[1:])):
        raise AssertionError("original receiver substep trace order incorrect")
    total = sum(v[12] for _, _, _, v in samples)
    audit = {
        "scope": f"original CALPUFF hour {args.hour} H3 receptor ({ix},{jy}), source-only",
        "vendor_sha256": original.ORIGINAL_CALPUFF_SHA256,
        "frozen_puff_con_sha256": result["puff_con_sha256"],
        "frozen_dgm_sha256": original.sha256_file(dgm),
        "records": len(samples),
        "record_bytes": RECORD.size,
        "trace_sha256": original.sha256_file(trace),
        "trace_path": str(trace),
        "projection_fraction_layout": "paired 3i13f CALCPF stage + 3i1f original FREC in unit97 stream",
        "first_receptor_contribution": {
            "seq": samples[0][0], "puff": samples[0][1],
            "ksam": samples[0][2],
            **dict(zip(FIELD_NAMES, samples[0][3])),
        },
        "trace_contribution_sum": total,
        "numeric_native_parity": "NOT_CLAIMED",
    }
    (out / (f"original-calcpf-h3-19-50.json" if args.hour==1 and ix==19 and jy==50 else
            f"original-calcpf-h3-{ix}-{jy}-hour{args.hour}.json")).write_text(
        json.dumps(audit, indent=2) + "\n")
    print("P7_ORIGINAL_H3_CALCPF_RECEIVER_ORACLE=PASS"
          f" HOUR={args.hour} RECEPTOR_I={ix} RECEPTOR_J={jy}"
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
