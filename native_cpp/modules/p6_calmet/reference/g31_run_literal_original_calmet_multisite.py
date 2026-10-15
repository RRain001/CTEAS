#!/usr/bin/env python3
"""G31A: execute LITERAL unmodified CALMET 6.5.0 on G19 2SS/2US source.

G19 provides original-format *synthetic* GEO/SURF/UP; only the copied met.inp
is adjusted for file locations, two UP file records and the single-hour
clock, which is independently taken from the G21 source first-hour trace.
The vendor Fortran bytes/COMMON physics and upstream G19 files are unchanged.

G31A is the independent original execution prerequisite, not yet a 45-field
scientific parity result. Never substitute a C++ candidate CALMET.DAT for the
original vendor output; failure to execute original CALMET is a HARD STOP.
"""
from __future__ import annotations
import argparse
import csv
import datetime as dt
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[4]
VENDOR=ROOT/"engine/vendor/src/CALMET_v6.5.0_L150223"
sys.path.insert(0,str(Path(__file__).resolve().parent))
EXPECTED_VENDOR_BLOB="437f43bb7f0e51a1919e56664663e1a046986ed0"

def sha256(file:Path)->str:
    return hashlib.sha256(file.read_bytes()).hexdigest()

def git_blob(raw:bytes)->str:
    return hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()

def source_clock(path:Path):
    with path.open(newline="") as stream:
        reader=csv.DictReader(stream)
        expected={"hour_end_utc_seconds","k_1based","j_1based","i_1based",
                  "u_mps_real4","v_mps_real4"}
        if set(reader.fieldnames or ())!=expected:
            raise ValueError("G31A first-hour source clock not genuine G21")
        stamps=set()
        count=0
        for row in reader:
            count+=1
            stamps.add(int(row["hour_end_utc_seconds"]))
    if count!=12*12*9 or len(stamps)!=1:
        raise ValueError("G31A source G21 did not contain one 1296-row hour")
    end=dt.datetime.fromtimestamp(stamps.pop(),dt.timezone.utc)
    start=end-dt.timedelta(hours=1)
    if any(t.minute or t.second for t in (start,end)):
        raise ValueError("G31A source hour not UTC on the hour")
    return start,end

def replace_control(deck:str,key:str,value:str)->str:
    # Source READCF control tokens are bracketed by ! !, no guessed defaults.
    pattern=re.compile(r"(!\s*"+re.escape(key)+r"\s*=\s*)([^!]*)(!)",re.I)
    matches=list(pattern.finditer(deck))
    if len(matches)!=1:
        raise ValueError(f"G31A ambiguous or missing source READCF {key}: {len(matches)}")
    m=matches[0]
    return deck[:m.start()]+m.group(1)+value+" "+m.group(3)+deck[m.end():]

def stage_deck(src:str,start:dt.datetime)->str:
    # All numerical READCF choices, station coordinates/identities,
    # model domain and physical scalar coefficients remain from G19.
    updates={
        "GEODAT":"geo.dat","SRFDAT":"surf.dat",
        "METLST":"met.lst","METDAT":"met.dat","LCFILES":"T",
        "IBYR":str(start.year),"IBMO":str(start.month),
        "IBDY":str(start.day),"IBHR":str(start.hour),"IBSEC":"0",
        # CALMET IE* is the timestamp of the *last* start of an
        # included hourly interval. IB*=IE* => exactly one hour.
        "IEYR":str(start.year),"IEMO":str(start.month),
        "IEDY":str(start.day),"IEHR":str(start.hour),"IESEC":"0"
    }
    result=src
    for key,val in updates.items():
        result=replace_control(result,key,val)
    matches=list(re.finditer(r"^UP1\.DAT[^\n]*!\s*UPDAT\s*=[^\n]*!END!",result,re.M|re.I))
    if len(matches)!=1:
        raise ValueError("G31A original 1UP file-list anchor has drifted")
    m=matches[0]
    line="UP1.DAT       input     1  ! UPDAT=up1.dat !    !END!"
    extra="UP2.DAT       input     2  ! UPDAT=up2.dat !    !END!"
    result=result[:m.start()]+line+"\n"+extra+result[m.end():]
    if result.count("UPDAT=")!=2:
        raise ValueError("G31A 2-UP vendor READCF group not exactly two")
    if src==result:
        raise ValueError("G31A source run deck not staged")
    # No interpolation or source-data fabrication is allowed here.
    for key in ("NSSTA","NUSTA","NX","NY","IUPT","IMIXH","IZICRLX",
                "CRITFN","TERRAD","ZIMIN","ZIMAX","CONSTN","CONSTE"):
        m=re.compile(r"!\s*"+key+r"\s*=\s*([^!]+)!")
        a=m.findall(src)
        b=m.findall(result)
        if len(a)!=1 or a!=b:
            raise ValueError("G31A changed protected physical source control "+key)
    return result

def original_compiler(source:Path,exe:Path,mode:str,compiler:str):
    include=exe.parent/"include-alias"
    include.mkdir(parents=True,exist_ok=True)
    for f in VENDOR.iterdir():
        if f.is_file():
            alias=include/f.name.lower()
            if not alias.exists():
                alias.symlink_to(f)
    flags=["-std=legacy","-ffixed-line-length-none","-fno-automatic",
           "-fno-align-commons","-fallow-argument-mismatch",
           "-O0" if mode=="Debug" else "-O2"]
    cmd=[compiler,*flags,"-I"+str(include),str(source),"-o",str(exe)]
    p=subprocess.run(cmd,capture_output=True,text=True,timeout=240)
    if p.returncode or not exe.is_file():
        raise RuntimeError("G31A literal original vendor compile failed\n"+p.stderr[-5000:])
    return cmd

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--mode",choices=["Debug","Release"],required=True)
    ap.add_argument("--source",type=Path,required=True,
                    help="G19 exported source folder with GEO.DAT, SURF.DAT, UP1/UP2.DAT, met.inp")
    ap.add_argument("--source-clock",type=Path,required=True)
    ap.add_argument("--out",type=Path,required=True)
    ap.add_argument("--compiler",default="gfortran")
    args=ap.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    evidence={"status":"NOT_EXECUTED","mode":args.mode,
              "scope":"G31A independent literal vendor first hour only; no 45-field parity"}
    report=args.out/"g31a-evidence.json"
    try:
        raw=(VENDOR/"calmet.for").read_bytes()
        evidence["original_vendor_blob"]=git_blob(raw)
        if evidence["original_vendor_blob"]!=EXPECTED_VENDOR_BLOB:
            raise RuntimeError("G31A original CALMET 6.5.0 source blob drift")
        clock_start,clock_end=source_clock(args.source_clock)
        evidence["source_hour_utc"]=[clock_start.isoformat(),clock_end.isoformat()]
        staged=args.out/"run"
        staged.mkdir(parents=True,exist_ok=True)
        copies={"GEO.DAT":"geo.dat","SURF.DAT":"surf.dat",
                "UP1.DAT":"up1.dat","UP2.DAT":"up2.dat"}
        evidence["unchanged_source_files"]={}
        for original,target in copies.items():
            p=args.source/original
            if not p.is_file() or not p.stat().st_size:
                raise RuntimeError("G31A missing actual G19 source file "+original)
            shutil.copy2(p,staged/target)
            if sha256(p)!=sha256(staged/target):
                raise RuntimeError("G31A original input bytes changed "+original)
            evidence["unchanged_source_files"][original]=sha256(p)
        met=(args.source/"met.inp")
        if not met.is_file():
            raise RuntimeError("G31A missing G19 original source controls")
        original_deck=met.read_text()
        result=stage_deck(original_deck,clock_start)
        (staged/"met.inp").write_text(result)
        evidence["original_G19_metinp_sha256"]=sha256(met)
        evidence["staged_vendor_metinp_sha256"]=sha256(staged/"met.inp")
        binary=args.out/"original-calmet.x"
        evidence["compiler_command"]=original_compiler(
            VENDOR/"calmet.for",binary,args.mode,args.compiler)
        proc=subprocess.run([str(binary),"met.inp"],cwd=staged,
                            capture_output=True,text=True,timeout=180)
        (args.out/"original-stdout.log").write_text(proc.stdout)
        (args.out/"original-stderr.log").write_text(proc.stderr)
        evidence["return_code"]=proc.returncode
        listing=staged/"met.lst"
        output=staged/"met.dat"
        evidence["original_listing_present"]=listing.exists()
        if listing.exists():
            evidence["original_listing_sha256"]=sha256(listing)
            evidence["original_listing_bytes"]=listing.stat().st_size
            shutil.copy2(listing,args.out/"original-met.lst")
        if proc.returncode:
            raise RuntimeError("G31A untouched original CALMET exited "+
                               str(proc.returncode)+"; inspect original-met.lst and original logs")
        if not output.is_file() or output.stat().st_size<=1024:
            raise RuntimeError("G31A original CALMET made no substantial MET.DAT")
        evidence["original_met_dat_sha256"]=sha256(output)
        evidence["original_met_dat_bytes"]=output.stat().st_size
        shutil.copy2(output,args.out/"original-met.dat")
        # The original vendor output is NEVER derived from the C++ binary.
        evidence["status"]="ORIGINAL_VENDOR_EXECUTED"
    except Exception as exc:
        evidence["status"]="BLOCKED"
        evidence["blocker"]=str(exc)
        raise
    finally:
        report.write_text(json.dumps(evidence,indent=2)+"\n")
        print(json.dumps(evidence,indent=2))

if __name__=="__main__":
    main()
