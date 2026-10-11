#!/usr/bin/env python3
"""Literal CALMET SIMILT L30097-30370 against C++ WIND1 selected neutral path.
IEXTRP=-4, EL=ZI=ZIMINI=10000. No claim of unstable/stable SIMILT parity.
"""
import argparse
import hashlib
import json
import math
import subprocess
from pathlib import Path

VENDOR_BLOB="437f43bb7f0e51a1919e56664663e1a046986ed0"
TOL_MPS=2e-4

DRIVER="""      program testsimilt
      include 'params.met'
      include 'grid.met'
      include 'met1.met'
      include 'ovrwat.met'
      real us(mxnz,mxwnd),vs(mxnz,mxwnd)
      real zz,rough,u0,v0,latitude
      real el,zi,zmini
      integer iex,ips
      character*256 input,output
      call get_command_argument(1,input)
      call get_command_argument(2,output)
      nx=50
      ny=50
      nz=9
      nzp1=10
      nssta=1
      nowsta=0
      zface(1)=0.
      zface(2)=20.
      zface(3)=40.
      zface(4)=80.
      zface(5)=160.
      zface(6)=320.
      zface(7)=640.
      zface(8)=1200.
      zface(9)=2000.
      zface(10)=3000.
      do k=1,nz
        zmid(k)=0.5*(zface(k+1)+zface(k))
      enddo
      open(10,file=trim(input),status='old')
      read(10,*) latitude,zz,rough,u0,v0,iex,ips
      close(10)
      if(iex.ne.-4)stop 72
      xslat(1)=latitude
      do k=1,nz
        us(k,1)=999999.
        vs(k,1)=999999.
      enddo
      us(1,1)=u0
      vs(1,1)=v0
      el=10000.
      zi=10000.
      zmini=10000.
      call similt(zz,el,rough,zi,1,ips,us,vs,zmini)
      open(11,file=trim(output),status='replace')
      do k=1,nz
        write(11,'(I5,2ES22.13E3)')k,us(k,1),vs(k,1)
      enddo
      close(11)
      end
      double precision function psiud(r)
      double precision r
      stop 78
      end
      real function psiuc(z,l,m)
      real z,l
      integer m
      stop 79
      end
"""

SCENES=[
    ("moderate_southwesterly",31.939774,10.,.1,3.,4.,0),
    ("eastward",31.9,10.,.03,5.,0.,0),
    ("northward",31.9,15.,.5,0.,7.,0),
    ("westward",31.9,10.,.01,-5.,0.,0),
    ("southward",31.9,10.,.1,0.,-6.,0),
    ("opposing_quadrant",31.9,12.,.005,-4.,-3.,0),
    ("positive_small",31.9,2.,.002,.01,.003,0),
    ("large_height",31.9,35.,.2,12.,-8.,0),
    ("rough_rural",31.9,10.,1.,1.5,2.7,0),
    ("low_roughness",31.9,10.,.0001,5.3,1.2,0),
    ("psi_option1_neutral",31.9,10.,.05,2.5,-4.1,1),
    ("latitude_high_north",60.,10.,.1,3.,4.,0),
]

def run(args):
    p=subprocess.run([str(x) for x in args],capture_output=True,text=True,timeout=100)
    if p.returncode:
        raise RuntimeError(f"command={args} exit={p.returncode} stdout={p.stdout[-800:]} stderr={p.stderr[-1800:]}")

def original_source(vendor,out):
    b=(vendor/"calmet.for").read_bytes()
    found=hashlib.sha1(f"blob {len(b)}\0".encode()+b).hexdigest()
    if found!=VENDOR_BLOB:raise ValueError(f"original CALMET blob drift {found}")
    lines=b.decode().splitlines(keepends=True)
    a=next(i for i,s in enumerate(lines) if s.lower().startswith("      subroutine similt("))
    end=next(i for i,s in enumerate(lines) if s.lower().startswith("      subroutine slope("))
    if a+1!=30097 or end+1!=30372:raise ValueError("original SIMILT source location drift")
    dest=out/"original_similt.for"
    dest.write_text("".join(lines[a:end]))
    return {"start_line":a+1,"end_line":end,
            "sha256":hashlib.sha256(dest.read_bytes()).hexdigest(),"file":dest}

def main():
    p=argparse.ArgumentParser()
    for key in ("vendor","cpp","out"):
        p.add_argument("--"+key,type=Path,required=True)
    args=p.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    original=original_source(args.vendor,args.out)
    driver=args.out/"driver.for";driver.write_text(DRIVER)
    binary=args.out/"original_similt"
    run(["gfortran","-std=legacy","-O0","-fno-automatic",
         "-ffixed-line-length-none","-fallow-argument-mismatch",
         "-I"+str(args.vendor),original["file"],driver,"-o",binary])
    comparisons=0
    maximum=0.
    errors=[]
    ranges=[]
    for name,lat,zan,z0,u,v,ips in SCENES:
        inp=args.out/(name+".in")
        inp.write_text(f"{lat} {zan} {z0} {u} {v} -4 {ips}\n")
        ref=args.out/(name+".orig")
        cpp=args.out/(name+".cpp")
        run([binary,inp,ref]);run([args.cpp,inp,cpp])
        original_rows=[line.split() for line in ref.read_text().splitlines()]
        modern_rows=[line.split() for line in cpp.read_text().splitlines()]
        if len(original_rows)!=9 or len(modern_rows)!=9:
            raise ValueError(f"{name}: unexpected layer count")
        max_case=0.
        for k,(a,b) in enumerate(zip(original_rows,modern_rows),1):
            if len(a)!=3 or len(b)!=3 or int(a[0])!=k or int(b[0])!=k:
                raise ValueError(f"{name}: layer/index mismatch at {k}")
            for comp,(x,y) in enumerate(zip(a[1:],b[1:])):
                r,m=float(x),float(y)
                if not(math.isfinite(r) and math.isfinite(m)):
                    raise ValueError(f"{name}: nonfinite at {k}")
                e=abs(r-m)
                comparisons+=1
                maximum=max(maximum,e)
                max_case=max(max_case,e)
                if e>TOL_MPS:errors.append(dict(scene=name,layer=k,component="UV"[comp],
                                                   original=r,cpp=m,error=e))
        ranges.append({"scene":name,"components":18,"max_abs_error_mps":max_case})
    report={"status":"PASS_SELECTED_NEUTRAL_SIMILT" if not errors else "FAIL",
            "scope":"WIND1 IEXTRP=-4 neutral ZMO=ZML=ZIMINI=10000, Northern Hemisphere surface, no overwater, not full WIND1",
            "vendor_blob_sha":VENDOR_BLOB,
            "fortran_source_sha256":original["sha256"],
            "source_lines":[original["start_line"],original["end_line"]],
            "tolerance_mps":TOL_MPS,
            "scenes":len(SCENES),"components_compared":comparisons,
            "max_abs_error_mps":maximum,"cases":ranges,"first_mismatches":errors[:12]}
    (args.out/"p6d-similt-neutral-parity.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))
    if errors:raise SystemExit(1)

if __name__=="__main__":main()
