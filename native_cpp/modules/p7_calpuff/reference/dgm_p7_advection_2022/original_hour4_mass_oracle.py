#!/usr/bin/env python3
"""Original write-only puff200 all-step H3 QM pre/post DRY precision oracle.

Compiles temporary Fortran source, keeps frozen 143h puff.con and DGM
SHA exact, and emits DGM-sequenced historical QM without feeding native C++.
"""
from __future__ import annotations
import argparse
import hashlib
import re
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
import original_puff_step_dump as base

RECORD=struct.Struct("<3i6f")
FULL_DGM_SHA="cbbdaa29c6222e6ca6aefe9f605ad13e7dd6d258f6ad243afcac07cb578108c6"
HELPERS="""
c --- Diagnostic-only COMP MASS COMMON, not part of source/physical COMMON.
      subroutine p7massstep(ii,ksam)
      integer ii,ksam,seq,seen,episode,substep
      real prior
      common /p7masdiag/ seq,seen,episode,substep,prior
      save seq
      data seq/0/
      seq=seq+1
      seen=0
      episode=ii
      substep=ksam
      return
      end
      subroutine p7massbefore(ii,ksam,oldmass)
      integer ii,ksam,seq,seen,episode,substep
      real oldmass,prior
      common /p7masdiag/ seq,seen,episode,substep,prior
      if(ii.ne.200) return
      if(episode.ne.ii.or.substep.ne.ksam) stop 92
      prior=oldmass
      seen=1
      return
      end
      subroutine p7massafter(ii,ksam,newmass,frac,vd,ratio,tsamp)
      integer ii,ksam,seq,seen,episode,substep,unit
      real newmass,frac,vd,ratio,tsamp,prior
      logical opened
      save opened,unit
      data opened/.false./
      common /p7masdiag/ seq,seen,episode,substep,prior
      if(ii.ne.200) return
      if(episode.ne.ii.or.substep.ne.ksam.or.seen.ne.1) stop 93
      if(.not.opened)then
         open(newunit=unit,file='dgm_mass_puff200.bin',
     &        access='stream',form='unformatted',status='replace')
         opened=.true.
      endif
      write(unit) seq,ii,ksam,prior,newmass,frac,vd,ratio,tsamp
      seen=2
      return
      end
"""

def main()->None:
    p=argparse.ArgumentParser()
    p.add_argument("--vendor",type=Path,required=True)
    p.add_argument("--case",type=Path,required=True)
    p.add_argument("--out",type=Path,required=True)
    p.add_argument("--compiler",default="gfortran-11")
    a=p.parse_args()
    vendor,case,out=(x.resolve() for x in (a.vendor,a.case,a.out))
    base.CALLS["item7_setpuf_return"] += "            call p7massstep(ii,ksam)\n"
    base.instrument(vendor,out)
    target=out/"calpuff_instrumented.for"
    source=target.read_text(encoding="latin-1")
    before="         if(mchem.ge.1)then"
    after="          call DEDAT(ndathrb,iyrss,ijulss,ihrss)"
    first_qold=source.index("            qold(ipl)=qm(ipl,ii)")
    first_qnew=source.index("            qnew(ipl)=qm(ipl,ii)")
    before_match=re.search(r"(?m)^ {6,}[ ]*if\(mchem\.ge\.1\)then",source[first_qold:first_qold+1900])
    after_match=re.search(r"(?m)^ {6,}[ ]*call DEDAT\(ndathrb,iyrss,ijulss,ihrss\)",
                          source[first_qnew:first_qnew+1400])
    if before_match is None or after_match is None:
        raise AssertionError(
            "original COMP QOLD/QNEW insert anchors drifted: "+
            f"before={before_match is not None} after={after_match is not None} "+
            f"qm_at={first_qold} qnew_at={first_qnew}")
    pos_before=first_qold+before_match.start()
    pos_after=first_qnew+after_match.start()
    source=(source[:pos_after]+
        "         call p7massafter(ii,ksam,qnew(1),fracdry(1),\n"
        "     &        vd(1),vdpvd(1),tsamp)\n"+source[pos_after:])
    source=(source[:pos_before]+
        "         call p7massbefore(ii,ksam,qold(1))\n"+source[pos_before:])
    target.write_text(source+HELPERS,encoding="latin-1")
    alias=base._alias_tree(vendor,out/"include-alias")
    executable=out/"bin"/"puff-original-mass.x"
    base._build(a.compiler,vendor/"modules.for",target,alias,executable)
    staged=out/"case"
    base._stage_case(case,staged)
    result=base._run(executable,staged/"run"/"puff")
    if result["puff_con_sha256"]!=base.TARGET_PUFF_CON_SHA256:
        raise AssertionError("original mass source instrumentation changed puff.con")
    dgm=staged/"run"/"puff"/"dgm_puffstep.bin"
    if hashlib.sha256(dgm.read_bytes()).hexdigest()!=FULL_DGM_SHA:
        raise AssertionError("original mass source instrumentation changed 143h DGM")
    trace=staged/"run"/"puff"/"dgm_mass_puff200.bin"
    data=trace.read_bytes()
    if len(data)%RECORD.size:
        raise AssertionError("original puff200 mass trace malformed")
    rows=[RECORD.unpack(data[i:i+RECORD.size])
          for i in range(0,len(data),RECORD.size)]
    if not rows or not any(r[0]==5847 and r[1]==200 and r[2]==25 for r in rows):
        raise AssertionError("real original puff200 row5847 not present")
    if len(set(row[0] for row in rows))!=len(rows):
        raise AssertionError("duplicated original physical DGM step")
    print(f"P7_ORIGINAL_HOUR4_MASS_HISTORY=PASS ROWS={len(rows)}"
          " PUFF=200 GLOBAL_DGM_5847_INCLUDED=YES"
          " FULL_143H_PUFF_CON_AND_DGM_SHA=UNCHANGED")
    for row in rows[:4]+[r for r in rows if r[0]==5847]:
        print("P7_ORIGINAL_PUFF200_MASS"
              f" SEQ={row[0]} PUFF={row[1]} KSAM={row[2]}"
              f" QOLD={row[3]:.9g} QNEW={row[4]:.9g}"
              f" FRACDRY={row[5]:.9g} VD={row[6]:.9g}"
              f" VDPVD={row[7]:.9g} TSAMP={row[8]:.9g}")

if __name__=="__main__":
    main()
