#!/usr/bin/env python3
"""P6D9A chained *unchanged-original* WIND1/INTER2/SMOOTH/DIVCEL/W parity.

Independently compiles frozen original routines, explicitly transmits
intermediate arrays through text fixtures and compares one-call C++ bridge.
This is a SYNTHETIC IDENTITY bypass of full FRADJ/heat/slope physics; no
actual Hefei 2022 U/V/W equivalence is inferred.
"""
import argparse
import csv
import hashlib
import json
import math
import subprocess
from pathlib import Path

import compare_original_wind1_firstguess as wind1
import compare_original_inter2 as inter2
import compare_original_diagno_final_segment as diag

SOURCE_BLOB="437f43bb7f0e51a1919e56664663e1a046986ed0"
NX=NY=50
NZ=9
NC=NX*NY
COMP_TOL=2e-4
DZ=(20.,20.,40.,80.,160.,320.,560.,800.,1000.)
SMOOTH=(0,1,2,0,1,2,0,1,0)
RADIUS=(.05,12.,12.,12.,.6,1.5)

def run(args):
    p=subprocess.run([str(x) for x in args],capture_output=True,text=True,timeout=180)
    if p.returncode:
        raise RuntimeError("command="+repr(args)+" return="+str(p.returncode)+
                           " stdout="+p.stdout[-900:]+" stderr="+p.stderr[-2800:])

def compile_references(vendor,out):
    base=["gfortran","-O0","-std=legacy","-fno-automatic",
          "-ffixed-line-length-none","-fallow-argument-mismatch",
          "-I"+str(vendor)]
    wind_sources=wind1.extract(vendor,out)
    orig_w=out/"d9_original_wind1"
    drv_w=out/"d9_original_wind1_driver.for"
    drv_w.write_text(wind1.DRIVER)
    run(base+wind_sources+[drv_w,"-o",orig_w])

    interp_src=inter2.extract_original(vendor,out)
    orig_i=out/"d9_original_inter2"
    drv_i=out/"d9_original_inter2_driver.for"
    drv_i.write_text(inter2.DRIVER)
    run(base+[s for s,_ in interp_src]+[drv_i,"-o",orig_i])

    seg_src,block,meta=diag.original_source(vendor,out)
    orig_d=out/"d9_original_final"
    drv_d=out/"d9_original_final_driver.for"
    drv_d.write_text(diag.DRIVER_HEAD+block+diag.DRIVER_TAIL)
    run(base+[s for s,_ in seg_src]+[drv_d,"-o",orig_d])
    return (orig_w,orig_i,orig_d),dict(
        wind1_original_sha=SOURCE_BLOB,
        inter2_original=[s for _,s in interp_src],
        final_original=meta)

def generate_synthetic(scene):
    name=scene["name"]
    land=[52 if (name=="upper_bias" and i<10) else 10
          for j in range(NY) for i in range(NX)]
    observation_weight=[
        0.5+0.125*((i+j+k)%3)
        for k in range(NZ) for j in range(NY) for i in range(NX)
    ]
    station_u=[
        (1.5+0.125*k if l==0 else 2.5+0.125*k)
        for k in range(NZ) for l in range(2)
    ]
    station_v=[
        (-2.0+0.0625*k if l==0 else 1.25+0.125*k)
        for k in range(NZ) for l in range(2)
    ]
    return land,observation_weight,station_u,station_v

def write_bridge(path,scene,land,weights,sta_u,sta_v):
    with path.open("w") as f:
        def line(values):
            f.write(" ".join(map(str,values))+"\n")
        line([scene["surface_x"],scene["surface_y"],
              scene["upper_x"],scene["upper_y"],scene["u"],scene["v"],
              scene["roughness"],scene["latitude"],scene["height"],
              scene["icalm"]])
        line(scene["bias"])
        for u,v in scene["upper"]:
            line([u,v])
        line(SMOOTH)
        line(DZ)
        line([2]*NZ)
        line(RADIUS)
        for row in range(0,len(land),NX):line(land[row:row+NX])
        for row in range(0,len(weights),NX):line(weights[row:row+NX])
        for k in range(NZ):line(sta_u[k*2:k*2+2])
        for k in range(NZ):line(sta_v[k*2:k*2+2])

def read_original_wind(path):
    rows=[line.split() for line in path.read_text().splitlines()]
    if len(rows)!=NC*NZ:raise RuntimeError("WIND1 original shape wrong")
    u=[];v=[]
    for idx,row in enumerate(rows):
        expected=(idx%NX+1,(idx//NX)%NY+1,idx//NC+1)
        if len(row)!=5 or tuple(map(int,row[:3]))!=expected:
            raise RuntimeError("WIND1 original record order invalid")
        u.append(float(row[3]))
        v.append(float(row[4]))
    return u,v

def write_original_inter2(ref_input,scene,land,weights,values,guess):
    # Use the unchanged INTER2 driver exactly as the independently
    # source-verified comparison script does; step1 comes from original WIND1.
    data=(scene["name"],2,False,list(RADIUS),[2]*NZ,
          [scene["surface_x"]*0.001,scene["upper_x"]*0.001],
          [scene["surface_y"]*0.001,scene["upper_y"]*0.001],
          land,values,guess,weights)
    inter2.write_input(ref_input,data)

def read_inter2_output(path):
    rows=path.read_text().splitlines()
    if len(rows)!=NC*NZ:raise RuntimeError("INTER2 output shape")
    x=[]
    for n,line in enumerate(rows):
        p=line.split()
        if len(p)!=4 or tuple(map(int,p[:3]))!=(
                n%NX+1,(n//NX)%NY+1,n//NC+1):
            raise RuntimeError("INTER2 grid ordering")
        x.append(float(p[3]))
    return x

def original_boundaries(u,v):
    ub=[];vb=[]
    for k in range(NZ):
        for side in range(2):
            i=0 if side==0 else NX-1
            ub.extend(u[k*NC+j*NX+i] for j in range(NY))
    for k in range(NZ):
        for side in range(2):
            j=0 if side==0 else NY-1
            vb.extend(v[k*NC+j*NX+i] for i in range(NX))
    return ub,vb

def compare_blend(u,v,modern):
    with modern.open(newline="") as f:rows=list(csv.DictReader(f))
    if len(rows)!=NC*NZ:raise RuntimeError("D9A C++ blended output dimensions")
    errors=[];largest=0.;changed=0
    for n,row in enumerate(rows):
        expected=(n%NX+1,(n//NX)%NY+1,n//NC+1)
        if tuple(int(row[k]) for k in ("i","j","k"))!=expected:
            raise RuntimeError("D9A blended grid mismatch")
        for name,orig in (("u",u[n]),("v",v[n])):
            val=float(row[name])
            if not math.isfinite(val):raise RuntimeError("nonfinite C++ blended field")
            error=abs(orig-val)
            largest=max(largest,error)
            if error>COMP_TOL and len(errors)<12:
                errors.append(dict(ijk=expected,component=name,error=error))
    return dict(components_compared=2*NC*NZ,
                max_abs_error_mps=largest,
                first_errors=errors,status="PASS" if not errors else "FAIL")

def main():
    p=argparse.ArgumentParser()
    for arg in ("vendor","cpp","out"):
        p.add_argument("--"+arg,type=Path,required=True)
    args=p.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    raw=(args.vendor/"calmet.for").read_bytes()
    sha=hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()
    if sha!=SOURCE_BLOB:raise RuntimeError("Original vendor CALMET source changed")
    (original_wind,original_inter,original_final),meta=compile_references(args.vendor,args.out)
    results={}
    # Independent original first guess, then two independent INTER2
    # components, then actual original final DIAGNO SMOOTH/DIVCEL/W block.
    selected={"unbiased","upper_bias","calm_icalm0"}
    for scene in wind1.scenes():
        name=scene["name"]
        if name not in selected:continue
        folder=args.out/name
        folder.mkdir(exist_ok=True)
        land,weights,station_u,station_v=generate_synthetic(scene)
        inp=folder/"bridge.in"
        write_bridge(inp,scene,land,weights,station_u,station_v)
        ref_w=folder/"wind1.original"
        cpp_w=folder/"wind1.cpp"
        cpp_blend=folder/"blend.cpp.csv"
        cpp_final=folder/"final.cpp.csv"
        cpp_bc=folder/"boundary.cpp"
        # The one-call C++ run never reads original output files.
        run([args.cpp,inp,cpp_w,cpp_blend,cpp_final,cpp_bc])
        run([original_w,inp,ref_w])
        comparison_first=wind1.compare(ref_w,cpp_w,name)
        u,v=read_original_wind(ref_w)

        observed={}
        for comp,field,first in (("u",station_u,u),("v",station_v,v)):
            config=folder/("inter2."+comp+".inp")
            dest=folder/("inter2."+comp+".original")
            write_original_inter2(config,scene,land,weights,field,first)
            run([original_inter,config,dest])
            observed[comp]=read_inter2_output(dest)
        comparison_blend=compare_blend(observed["u"],observed["v"],cpp_blend)

        bc_u,bc_v=original_boundaries(u,v)
        original_d_in=folder/"final.original.inp"
        original_d=folder/"final.original"
        original_bc=folder/"boundary.original"
        diag.write_case(original_d_in,list(SMOOTH),[
            observed["u"],observed["v"],bc_u,bc_v])
        run([original_final,original_d_in,original_d,original_bc])
        comparison_final=diag.compare(original_d,cpp_final,original_bc,cpp_bc)
        results[name]=dict(first_guess=comparison_first,
                           inter2=comparison_blend,final=comparison_final)

    complete=(len(results)==len(selected) and
        all(
          r["first_guess"]["status"]=="PASS" and
          r["inter2"]["status"]=="PASS" and
          r["final"]["status"]=="PASS"
          for r in results.values()))
    counters=dict(
        firstguess_components=sum(r["first_guess"]["components_compared"]
                                  for r in results.values()),
        blended_components=sum(r["inter2"]["components_compared"]
                               for r in results.values()),
        final_uv_div_w_components=sum(r["final"]["uv_div_w_comparisons"]
                                      for r in results.values()),
        boundary_components=sum(r["final"]["boundary_comparisons"]
                                for r in results.values())
    )
    report=dict(
        status="PASS_D9A_SELECTED_CHAINED_ORIGINAL_SOURCES" if complete else "FAIL",
        scope="synthetic identity upstream WIND1 original -> original INTER2 U/V -> original SMOOTH/DIVCEL/W; no FRADJ/heat/slope, not real 2022 full DIAGNO",
        vendor_blob=sha,original_reference=meta,
        tolerance_wind_mps=COMP_TOL,summary=counters,scenes=results)
    (args.out/"p6d9a-original-chained-source-parity.json").write_text(
        json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))
    if not complete:raise SystemExit(1)

if __name__=="__main__":
    main()
