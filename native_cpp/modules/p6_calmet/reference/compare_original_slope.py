#!/usr/bin/env python3
"""Independent literal-CALMET-6.5.0 SLOPE vs C++ numerical comparison.
No vendor edits, no synthetic expected fields masquerading as Fortran truth.
SLOPE-only inputs explicitly supply QH: P6E has not implemented HEATFX.
"""
import argparse
import csv
import hashlib
import json
import math
import subprocess
from pathlib import Path

EXPECTED_GIT_BLOB = "437f43bb7f0e51a1919e56664663e1a046986ed0"
ABS_TOL_MPS = 2.0e-4  # declared before comparison; REAL*4 original
NX, NY, NZ = 50, 50, 9
N = NX * NY

DRIVER = """      program checkslope
      include 'params.met'
      include 'grid.met'
      include 'geo.met'
      include 'metgrd.met'
      include 'd6.met'
      real htopo(mxnx,mxny),hmax(mxnx,mxny),temp2d(mxnx,mxny)
      real uslope(mxnx,mxny,mxnz),vslope(mxnx,mxny,mxnz)
      character*256 fin,fout,arg
      call get_command_argument(1,fin)
      call get_command_argument(2,fout)
      call get_command_argument(3,arg)
      read(arg,*) isurft
      nx=50
      ny=50
      nz=9
      dx=100.
      dy=100.
      tinf=285.
      iwr=0
      iwat1=50
      iwat2=55
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
      open(10,file=trim(fin),status='old')
      do j=1,ny
        read(10,*) (htopo(i,j),i=1,nx)
      enddo
      do j=1,ny
        read(10,*) (hmax(i,j),i=1,nx)
      enddo
      do j=1,ny
        read(10,*) (hmin(i,j),i=1,nx)
      enddo
      do j=1,ny
        read(10,*) (disthmax(i,j),i=1,nx)
      enddo
      do j=1,ny
        read(10,*) (disthmin(i,j),i=1,nx)
      enddo
      do j=1,ny
        read(10,*) (qh(i,j),i=1,nx)
      enddo
      do j=1,ny
        read(10,*) (temp2d(i,j),i=1,nx)
      enddo
      do j=1,ny
        read(10,*) (ilandu(i,j),i=1,nx)
      enddo
      close(10)
      call slope(uslope,vslope,htopo,isurft,tinf,temp2d,hmax)
      open(11,file=trim(fout),status='replace')
      do k=1,nz
        do j=1,ny
          do i=1,nx
            write(11,'(3(i4,1x),2(es22.13e3,1x))')
     1        i,j,k,uslope(i,j,k),vslope(i,j,k)
          enddo
        enddo
      enddo
      close(11)
      end
      subroutine xmit(n,value,array)
      integer n
      real value,array(*)
      do i=1,abs(n)
        array(i)=value
      enddo
      end
      subroutine wndlpt(array)
      real array(*)
      return
      end
"""

def run(command, timeout=90):
    p = subprocess.run([str(x) for x in command], capture_output=True,
                       text=True, timeout=timeout)
    if p.returncode:
        raise RuntimeError(f"{command} returned {p.returncode}\n"
                           f"stdout={p.stdout[-1200:]}\nstderr={p.stderr[-3000:]}")

def original_extracted(vendor, out):
    source = vendor / "calmet.for"
    git_sha = subprocess.check_output(
        ["git", "hash-object", str(source)], text=True).strip()
    if git_sha != EXPECTED_GIT_BLOB:
        raise ValueError(f"vendor source SHA mismatch: {git_sha}")
    lines = source.read_text().splitlines(keepends=True)
    def anchor(name):
        a = [i for i, line in enumerate(lines)
             if line.lower().startswith(f"      subroutine {name}(")]
        if len(a) != 1:
            raise ValueError(f"original {name} anchor ambiguity {a}")
        return a[0]
    lo, hi = anchor("slope"), anchor("smooth")
    if not lo < hi:
        raise ValueError("invalid original subroutine order")
    part = out / "literal_original_slope.for"
    part.write_text("".join(lines[lo:hi]))
    return {"vendor_blob": git_sha, "first_line": lo + 1,
            "last_line": hi, "source_sha256": hashlib.sha256(
                part.read_bytes()).hexdigest(), "file": part}

def cases():
    # Declared land categories 50..55 represent water in this isolated test.
    flat = [100.0] * N
    ridge = [100 + 0.6*i + 0.14*j +
             24 * math.exp(-((i-21)**2+(j-24)**2)/90)
             for j in range(NY) for i in range(NX)]
    valley = [130 - 0.42*i + 0.33*j -
              20 * math.exp(-((i-25)**2+(j-15)**2)/120)
              for j in range(NY) for i in range(NX)]
    for name, h, mode, isurft in (
        ("flat_no_slope", flat, "night", -1),
        ("ridge_night_negative_qh", ridge, "night", -1),
        ("ridge_day_positive_qh", ridge, "day", -1),
        ("valley_mixed_water_ambient_t", valley, "mixed", 1)):
        high = [x+100.0 for x in h]
        low = [x-80.0 for x in h]
        dist_high = [220+3*(i%13) for i in range(N)]
        dist_low = [190+5*(i%11) for i in range(N)]
        if mode == "night":
            flux = [-80-25*math.sin(i*.13) for i in range(N)]
        elif mode == "day":
            flux = [100+40*math.cos(i*.09) for i in range(N)]
        else:
            flux = [(-65 if i%3 else 90) for i in range(N)]
        temp = [279+3*math.sin(i*.1) for i in range(N)]
        land = [52 if mode=="mixed" and i%7==0 else 10 for i in range(N)]
        yield name, [h, high, low, dist_high, dist_low, flux, temp, land], isurft

def write_case(path, fields):
    with path.open("w") as f:
        for ix, field in enumerate(fields):
            if len(field) != N:raise ValueError("not 50x50")
            for j in range(0,N,NX):
                if ix == 7:
                    f.write(" ".join(str(v) for v in field[j:j+NX])+"\n")
                else:
                    f.write(" ".join(f"{v:.7f}" for v in field[j:j+NX])+"\n")

def compare(original, modern, land):
    a = [list(map(float,line.split())) for line in original.read_text().splitlines()]
    with modern.open(newline="") as f:
        b = list(csv.DictReader(f))
    if len(a)!=N*NZ or len(b)!=N*NZ:
        raise ValueError(f"incorrect 3D row count Fortran {len(a)} C++ {len(b)}")
    maxerr=0.0
    exceed=[]
    nonzero=0
    for n,(v,w) in enumerate(zip(a,b)):
        point=n%N
        i,j,k=point%NX+1,point//NX+1,n//N+1
        if len(v)!=5 or tuple(map(int,v[:3]))!=(i,j,k) or (
           int(w["i"]),int(w["j"]),int(w["k"]))!=(i,j,k):
            raise ValueError(f"row ordering invalid: {i,j,k}")
        for orig,cand in ((v[3],float(w["u_mps"])),(v[4],float(w["v_mps"]))):
            if not math.isfinite(orig) or not math.isfinite(cand):
                raise ValueError("nonfinite SLOPE output")
            nonzero += (abs(orig)>1e-6)
            if land[point]==52 and (orig!=0 or cand!=0):
                raise ValueError("water-cell flow nonzero")
            err=abs(orig-cand)
            maxerr=max(err,maxerr)
            if err>ABS_TOL_MPS and len(exceed)<12:
                exceed.append({"i":i,"j":j,"k":k,
                               "original":orig,"cpp":cand,"err":err})
    return {"components_compared":N*NZ*2, "nonzero_original_components":nonzero,
            "max_abs_error_mps":maxerr,"tolerance_mps":ABS_TOL_MPS,
            "first_exceedances":exceed, "status":"PASS" if not exceed else "FAIL"}

def main():
    parser=argparse.ArgumentParser()
    for name in ("vendor","cpp","out"):
        parser.add_argument("--"+name, type=Path, required=True)
    args=parser.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    extracted=original_extracted(args.vendor,args.out)
    driver=args.out/"slope_driver.for"
    driver.write_text(DRIVER)
    exe=args.out/"original_slope"
    run(["gfortran","-O0","-std=legacy","-fno-automatic",
         "-fno-align-commons","-ffixed-line-length-none",
         "-I"+str(args.vendor),extracted["file"],driver,"-o",exe])
    results={}
    for name,fields,isurft in cases():
        infile=args.out/(name+".inp")
        orig=args.out/(name+".fortran")
        modern=args.out/(name+".cpp.csv")
        write_case(infile,fields)
        run([exe,infile,orig,isurft])
        run([args.cpp,infile,modern,isurft])
        results[name]=compare(orig,modern,fields[7])
    report={"stage":"P6D_SLOPE_ISOLATED_ONLY",
            "source":{k:v for k,v in extracted.items() if k!="file"},
            "cases":results,
            "total_components":sum(x["components_compared"] for x in results.values()),
            "status":"PASS" if all(x["status"]=="PASS" for x in results.values()) and
                   results["ridge_night_negative_qh"]["nonzero_original_components"]>0 and
                   results["ridge_day_positive_qh"]["nonzero_original_components"]>0
                   else "FAIL"}
    (args.out/"slope-parity.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))
    if report["status"]!="PASS":
        raise SystemExit(1)

if __name__=="__main__":
    main()
