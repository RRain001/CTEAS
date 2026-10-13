#!/usr/bin/env python3
"""Read-only DIAGNO SURFVAR/AVETMP input audit on original 143h source.

Emit pre/post AVETMP original U,V,T fields at the DIAGNO IALL=0 call.
Cross-check the pinned unmodified original CALMET.DAT is byte-identical;
then compare against independent native WIND1 first guess and TEMP2D.
Never feed original winds or temperatures back into the native model.
"""
from __future__ import annotations
import argparse,datetime as dt,hashlib,json,math,struct,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"native_cpp/modules/p6_calmet/reference"))
import original_internal_state_dump as dgm

NX=NY=50
NCELLS=2500
NH=143
MARKER="""      if (iavet.eq.1.and.iall.ne.2) 
     &  call avetmp(nx,ny,mnmdav,hafang,dgrid,u,v,temp2d)
"""
HELPER="""
c --- Diagnostic read-only pre/post AVETMP vectors in DIAGNO IALL=0
      subroutine dgmavetrace(point,u,v,t)
      include 'params.met'
      include 'gen.met'
      include 'grid.met'
      include 'metgrd.met'
      integer point,i,j
      real u(mxnx,mxny),v(mxnx,mxny),t(mxnx,mxny)
      logical opened
      save opened
      data opened/.false./
      if(.not.opened)then
         open(93,file='dgm_avetmp_inputs.bin',form='unformatted',
     1        access='stream',status='replace')
         opened=.true.
      endif
      write(93) point,ndathrb,nsecb,nx,ny
      write(93) ((u(i,j),i=1,nx),j=1,ny)
      write(93) ((v(i,j),i=1,nx),j=1,ny)
      write(93) ((t(i,j),i=1,nx),j=1,ny)
      return
      end
"""
def sha(p:Path)->str:
    return hashlib.sha256(p.read_bytes()).hexdigest()
def source_to_instrumented(p:Path,out:Path):
    s=p.read_text()
    if s.count(MARKER)!=1 or "      call dgmavetrace(" in s:
        raise RuntimeError("AVETMP actual source anchor not unique")
    s=s.replace(MARKER,
        "      if (iall.eq.0) call dgmavetrace(1,u,v,temp2d)\n"+
        MARKER+
        "      if (iall.eq.0) call dgmavetrace(2,u,v,temp2d)\n",1)
    out.write_text(s+HELPER)
def trace(p:Path):
    recsize=20+3*NCELLS*4
    raw=p.read_bytes()
    if len(raw)!=NH*2*recsize:
        raise RuntimeError(f"unexpected Fortran AVETMP trace bytes {len(raw)}")
    for h in range(NH):
        values={}
        for step in (1,2):
            off=(h*2+(step-1))*recsize
            point,packed,sec,nx,ny=struct.unpack_from("<5i",raw,off)
            t=dt.datetime(2022,1,1)+dt.timedelta(hours=h)
            want=t.year*100000+t.timetuple().tm_yday*100+t.hour
            if (point,packed,sec,nx,ny)!=(step,want,0,NX,NY):
                raise RuntimeError(f"wrong Fortran hour/point {(point,packed,sec,nx,ny)}")
            x=struct.unpack_from("<7500f",raw,off+20)
            if not all(math.isfinite(v) and abs(v)<999998 for v in x):
                raise RuntimeError("Fortran input nonfinite/missing")
            values[step]=(x[:NCELLS],x[NCELLS:2*NCELLS],x[2*NCELLS:])
        yield values
def native(p:Path,label:str,stem:str):
    dat=(p/f"{stem}_{label}.f32").read_bytes()
    if len(dat)<NCELLS*4:raise RuntimeError("native short field "+stem+" "+label)
    return struct.unpack_from("<2500f",dat)
def summarize():
    return {"compared":0,"nonzero":0,"max_abs":0.,"worst":None,"exceed_1e_minus_4":0}
def feed(stats,orig,cand,label:str):
    for index,(o,c) in enumerate(zip(orig,cand)):
        delta=abs(o-c)
        stats["compared"]+=1
        if delta!=0:stats["nonzero"]+=1
        if delta>1e-4:stats["exceed_1e_minus_4"]+=1
        if delta>stats["max_abs"]:
            stats["max_abs"]=delta
            stats["worst"]={"hour":label,"i":index%50+1,"j":index//50+1,
                             "original":o,"native":c,"absolute_difference":delta}
def main():
    ap=argparse.ArgumentParser()
    for name in ("original-dir","vendor","case","candidate","out"):
        ap.add_argument("--"+name,type=Path,required=True)
    ap.add_argument("--compiler",default="gfortran-11")
    args=ap.parse_args()
    audit=json.loads((args.original_dir/"dgm-audit.json").read_text())
    if audit.get("status")!="ORIGINAL_MET_DAT_IDENTICAL" or (
        audit.get("frozen_reference_sha256")!=dgm.TARGET_MET_DAT_SHA256
    ) or sha(args.vendor/"calmet.for")!=dgm.ORIGINAL_CALMET_SHA256:
        raise RuntimeError("reference original not frozen/audited")
    original=args.original_dir/"calmet_instrumented.for"
    if sha(original)!=audit["instrumentation"]["instrumented_sha256"]["calmet_instrumented.for"]:
        raise RuntimeError("original audit not intact")
    work=args.out.parent/"avetmp_input_original_build"
    work.mkdir(parents=True,exist_ok=True)
    src=work/"calmet_avetmp_input_instrumented.for"
    source_to_instrumented(original,src)
    exe=work/"bin"/"calmet_original_avetmp_inputs.x"
    dgm._build(args.compiler,src,args.original_dir/"include-alias",exe)
    staged=work/"case"
    dgm._stage_case(args.case.resolve(),staged)
    done=dgm._run(exe,staged/"run/met")
    if done["met_dat_sha256"]!=dgm.TARGET_MET_DAT_SHA256:
        raise RuntimeError("read-only AVETMP instrumentation changed original MET.DAT")
    stats={k:summarize() for k in (
        "pre_avetmp_u","pre_avetmp_v","pre_avetmp_t","post_avetmp_t")}
    focus=[]
    for h,st in enumerate(trace(staged/"run/met"/"dgm_avetmp_inputs.bin")):
        stamp=dt.datetime(2022,1,1)+dt.timedelta(hours=h+1)
        label=stamp.strftime("%Y-%m-%d_%H")
        pre,post=st[1],st[2]
        native_u=native(args.candidate,label,"wind1_u")
        native_v=native(args.candidate,label,"wind1_v")
        native_post=native(args.candidate,label,"temp2d")
        # Native selected-case station has one uniform surface temperature.
        # First inspect how original SURFVAR constructed its pre-AVETMP field.
        feed(stats["pre_avetmp_u"],pre[0],native_u,label)
        feed(stats["pre_avetmp_v"],pre[1],native_v,label)
        # The before/after values in ORIGINAL expose all actual observed T.
        # The native observed T may be independently checked from SURF.DAT.
        feed(stats["post_avetmp_t"],post[2],native_post,label)
        if label in ("2022-01-01_19","2022-01-02_18","2022-01-06_12"):
            idx=(19-1)*50+(12-1)
            focus.append({
                "hour":label,"point":[12,19],
                "original_pre_u":pre[0][idx],"native_pre_u":native_u[idx],
                "original_pre_v":pre[1][idx],"native_pre_v":native_v[idx],
                "original_pre_temperature":pre[2][idx],
                "original_post_temperature":post[2][idx],
                "native_post_temperature":native_post[idx],
                "original_pre_t_minmax":[min(pre[2]),max(pre[2])],
                "original_pre_t_unique":len(set(pre[2])),
                "original_post_t_max_abs_from_pre":
                    max(abs(x-y) for x,y in zip(post[2],pre[2])),
            })
    report={"status":"FORENSIC_ONLY_NO_MODEL_INPUT_SUBSTITUTION",
        "original_met_unchanged":True,
        "original_vendor_sha256":dgm.ORIGINAL_CALMET_SHA256,
        "instrumented_source_sha256":sha(src),
        "original_trace_sha256":sha(staged/"run/met"/"dgm_avetmp_inputs.bin"),
        "source":"DIAGNO SURFVAR IALL=0 pre/post actual AVETMP",
        "native":"selected C++ WIND1 first-guess and TEMP2D",
        "fields":stats,"focus":focus}
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))
if __name__=="__main__":main()
