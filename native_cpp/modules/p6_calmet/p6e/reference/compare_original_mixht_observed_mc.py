#!/usr/bin/env python3
"""Pinned unchanged MIXHT+MIXDT+MIXHMC full daytime/night selected path.

The entire three original subroutines are compiled from the pinned
original unchanged Fortran source. MIXHBG, MIXDT2 and OUT fail-stop
stubs deliberately rule out unsupported pathways. Not COMP/2022 parity.
"""
import argparse
import hashlib
import json
import math
import subprocess
import tempfile
from pathlib import Path
BLOB="437f43bb7f0e51a1919e56664663e1a046986ed0"
CASES,NX,NY,NS,NLEV=28,5,4,2,10
ABS=(0.07,0.07,0.004)
REL=0.00008
PARAMS="""      integer mxnx,mxny,mxus,mxlev,io6
      parameter(mxnx=5,mxny=4,mxus=2,mxlev=10,io6=6)
"""
DRIVER="""      program original_mixht_mc_observed
      include 'params.met'
      include 'ziparm.met'
      include 'upmet.met'
      include 'gen.met'
      real qh(mxnx,mxny),ustar(mxnx,mxny),el(mxnx,mxny)
      real rho(mxnx,mxny),zi(mxnx,mxny),ziconv(mxnx,mxny)
      real dptt(mxnx,mxny)
      common /tjump/dptt
      integer ilandu(mxnx,mxny),nearu(mxnx,mxny)
      integer cases,nx,ny,ns,nlev,iupt,imixh
      integer c,i,j,st,k,b,watlow,wathigh
      logical ldbhr
      read(*,*)cases,nx,ny,ns,nlev
      if(nx.ne.5.or.ny.ne.4.or.ns.ne.2.or.nlev.ne.10)stop 66
      imixh=1
      watlow=50
      wathigh=60
      ldbhr=.false.
      do c=1,cases
        read(*,*)iupt,constn,zimin,zimax,threshl,dptmin,dzzi,
     1           onedte,twodte,izicrlx,tzicrlx,nsecdt
        do st=1,ns
          read(*,*)justa(st)
          nlaa(st)=nlev
          nlbb(st)=nlev
        enddo
        do b=1,2
          do st=1,ns
            do k=1,nlev
              if(b.eq.1)then
                read(*,*)zlaa(st,k),tzaa(st,k)
              else
                read(*,*)zlbb(st,k),tzbb(st,k)
              endif
            enddo
          enddo
        enddo
        do i=1,nx
          do j=1,ny
            read(*,*)qh(i,j),ustar(i,j),el(i,j),rho(i,j),
     1        cmech(i,j),fcori(i,j),ilandu(i,j),nearu(i,j),
     2        zi(i,j),ziconv(i,j),dptt(i,j)
          enddo
        enddo
        call mixht(el,ustar,qh,nx,ny,rho,12,nearu,
     1        iupt,ilandu,watlow,wathigh,ldbhr,imixh,zi,ziconv)
        do i=1,nx
          do j=1,ny
            write(*,'(3(I6,1X),3(ES24.14E3,1X))')
     1        c,i,j,zi(i,j),ziconv(i,j),dptt(i,j)
          enddo
        enddo
      enddo
      end
      subroutine mixhbg
      stop 91
      end
      subroutine mixdt2
      stop 92
      end
      subroutine out
      stop 93
      end
"""
def run(command,inp=None):
    return subprocess.run([str(x) for x in command],input=inp,
                          text=True,capture_output=True,check=True).stdout
def source(raw):
    rows=raw.decode("utf-8").splitlines(keepends=True)
    specs=(("      subroutine mixht(","      subroutine mixhtst(",230,320),
           ("      subroutine mixdt(","      subroutine mixdt2(",185,220),
           ("      subroutine mixhmc(","      subroutine openot",150,240))
    portions=[];loc=[]
    for first,last,low,high in specs:
        aa=[i for i,v in enumerate(rows) if v.lower().startswith(first)]
        bb=[i for i,v in enumerate(rows) if v.lower().startswith(last)]
        if len(aa)!=1 or len(bb)!=1 or not low<bb[0]-aa[0]<high:
            raise RuntimeError("CALMET original subroutine anchor drift "+first)
        portions.append("".join(rows[aa[0]:bb[0]]))
        loc.append([aa[0]+1,bb[0]])
    return "\n".join(portions),loc
def fixture():
    heights=(0.,50.,100.,180.,300.,450.,650.,900.,1250.,1800.)
    qh=(-0.1,0.,5.,35.,90.,160.,400.)
    ht=(80.,120.,240.,400.)
    rows=[f"{CASES} {NX} {NY} {NS} {NLEV}"]
    counts=dict(water=0,stable=0,day=0,aa=0,bb=0,iupt_override=0)
    for c in range(CASES):
        iupt=2 if c%3==0 else -1
        counts["iupt_override"]+=iupt>0
        zimin=20.+5.*(c%2);zimax=1500.+100.*(c%5)
        thresh=(0.,0.02,0.10)[c%3]
        relax=c%2
        rows.append(f"{iupt} 2400.0 {zimin} {zimax} {thresh} "
                    f"0.001 200. 4140. 1080. {relax} 800. 3600")
        justa=[1 if (st+c)%2==0 else -1 for st in range(NS)]
        for x in justa:rows.append(str(x))
        for buf in range(2):
            for st in range(NS):
                for k,h in enumerate(heights):
                    z=h
                    temp=303.-0.0075*z+0.9*st+buf*1.8+c*0.01+0.1*(k%3)
                    rows.append(f"{z:.4f} {temp:.6f}")
        for i in range(NX):
            for j in range(NY):
                k=i+2*j+c
                # Deliberately avoid coupling water-mask periodicity to
                # the QH stable (-0.1) period of 7; that masked every
                # QH<0 cell in the initial nominal joint test.
                use=55 if (i+2*j+c)%9==0 else 12
                heat=qh[k%len(qh)]
                if use==55:counts["water"]+=1
                elif heat<0:counts["stable"]+=1
                else:counts["day"]+=1
                near=1+((i+j+c)%NS)
                station=(iupt-1) if iupt>0 else near-1
                oldbuffer=0 if justa[station]>0 else 1
                counts["aa" if oldbuffer==0 else "bb"]+=use!=55 and heat>=0
                us=(0.12,0.22,0.4,0.8)[(i+j+c)%4]
                el=(15.,40.,100.,200.)[(i+2*j+c)%4]
                rho=(1.05,1.14,1.2,1.3)[(i+j+c)%4]
                zic=ht[(i+3*j+c)%4] if use!=55 else -7777.
                jump=(0.,0.2,0.5,1.)[(i+2*j+c)%4] if use!=55 else -7777.
                rows.append(f"{heat} {us} {el} {rho} "
                            f"{90.+10.*(i%3)} 0.00007 {use} {near} "
                            f"-7777.0 {zic} {jump}")
    if min(counts["stable"],counts["day"],counts["water"],
           counts["aa"],counts["bb"],counts["iupt_override"]) < 10:
        raise RuntimeError("MIXHT MC branch coverage insufficient")
    return "\n".join(rows)+"\n",counts
def parse(txt):
    out=[]
    for line in txt.splitlines():
        a=line.split()
        if len(a)!=6:raise RuntimeError("MIXHT MC invalid comparator schema "+line)
        key=tuple(int(x) for x in a[:3]);val=tuple(float(x) for x in a[3:])
        if not all(math.isfinite(x) for x in val):
            raise RuntimeError("MIXHT MC original nonfinite value")
        out.append((key,val))
    if len(out)!=CASES*NX*NY:
        raise RuntimeError(f"MIXHT MC original output size {len(out)}")
    return out
def main():
    p=argparse.ArgumentParser()
    for flag in ("vendor","cpp","out"):p.add_argument("--"+flag,type=Path,required=True)
    x=p.parse_args()
    raw=x.vendor.read_bytes()
    sha=run(["git","hash-object",x.vendor]).strip()
    if sha!=BLOB:raise RuntimeError("Original CALMET vendor blob changed")
    fortran,loc=source(raw)
    test,counts=fixture()
    with tempfile.TemporaryDirectory(prefix="p6e-mixht-mc-") as tmp:
        d=Path(tmp)
        for name,data in (("params.met",PARAMS),
                          ("ziparm.met",(x.vendor.parent/"ziparm.met").read_text()),
                          ("upmet.met",(x.vendor.parent/"upmet.met").read_text()),
                          ("gen.met",(x.vendor.parent/"gen.met").read_text()),
                          ("original.for",fortran),("driver.for",DRIVER)):
            (d/name).write_text(data)
        run(["gfortran","-O0","-std=legacy","-fno-automatic",
             "-ffixed-line-length-none","-I"+str(d),
             d/"original.for",d/"driver.for","-o",d/"original"])
        expected=parse(run([d/"original"],test))
    actual=parse(run([x.cpp],test))
    errs=[0.,0.,0.];first=[];bad=0
    for (a,f),(b,v) in zip(expected,actual):
        if a!=b:raise RuntimeError("MIXHT MC output order mismatch")
        water=f[0]==-7777.
        for k in range(3):
            err=abs(f[k]-v[k]);errs[k]=max(errs[k],err)
            fail=(f[k]!=v[k]) if water else err>ABS[k]+REL*abs(f[k])
            if fail:
                bad+=1
                if len(first)<25:first.append(
                    dict(grid=a,field=k,original=f[k],cpp=v[k],absdiff=err))
    result=dict(status="PASS" if bad==0 else "FAIL",
        cases=CASES,grids=CASES*NX*NY,scalar_comparisons=3*CASES*NX*NY,
        distribution=counts,max_abs_error_ZI_ZICONV_DPTT=errs,
        frozen_abs_tolerance=ABS,frozen_rel_tolerance=REL,
        mismatches=bad,first_mismatches=first,
        original_git_blob=sha,original_sha256=hashlib.sha256(raw).hexdigest(),
        unmodified_original_subroutines_lines=loc,
        scope="Selected CALMET full MIXHT grid path IMIXH=1, AA/BB JUSTA, IUPT; synthetic only, no BG, no 2022 COMP or wet forcing")
    x.out.parent.mkdir(parents=True,exist_ok=True)
    x.out.write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps(result,indent=2))
    if bad:raise SystemExit(1)
if __name__=="__main__":main()
