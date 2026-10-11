#!/usr/bin/env python3
"""P6C original CALMET COMP runtime AA/BB observation-state audit.

No edits to vendor source. --instrument copies original Fortran to a temporary
file and adds a write-only state trace immediately before the source's validated
COMP return. --compare checks emitted runtime state for all 143 source hours
against the independent C++ joint trace and 13 original UP.DAT timestamps.
Original CALMET output SHA equivalence is independently checked by workflow.
"""
from __future__ import annotations
import argparse,csv,datetime,hashlib,json,re
from pathlib import Path

ORIGINAL_SHA="430381b4fb87f15127e20b13cc3e99be8a0e0fccf1d5821110af7f980558d134"
ANCHOR="      go to 140"
ADD="""      if(iu.eq.1) write(6,*)'P6C_COMP_TRACE',
     & nowtze,nsecze,ntzaa(iu),ntzbb(iu),justa(iu),
     & jusdt(iu),jaasec(iu),jbbsec(iu)
"""
def sha(p:Path):return hashlib.sha256(p.read_bytes()).hexdigest()
def instrument(source:Path,out:Path):
    if sha(source)!=ORIGINAL_SHA:
        raise ValueError("vendor source SHA drift; trace insertion not permitted")
    lines=source.read_text().splitlines(keepends=True)
    if len(lines)<4412 or lines[4267].rstrip()!=ANCHOR or       "if(abs(iupsec).lt.nsecdt.and.iupsec.ne.0) then" not in lines[4150] or       "if(jdelta.gt.maxsep*3600) then" not in lines[4235] or       "justa(iu) = -1" not in lines[4365] or       "justa(iu) = +1" not in lines[4292]:
        raise ValueError("source COMP anchors do not match pinned v6.5.0")
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text("".join(lines[:4267])+ADD+"".join(lines[4267:]))
    report={"status":"ORIGINAL_COMP_WRITE_ONLY_TRACE_INSTRUMENTED",
            "source_sha256":sha(source),"instrumented_sha256":sha(out),
            "original_comp_source_lines":[4118,4412],
            "traced_fields":["nowtze","nsecze","ntzaa","ntzbb",
                             "justa","jusdt","jaasec","jbbsec"],
            "no_vendor_source_edits":True,
            "original_timestep_gate":"abs(iupsec)<nsecdt and iupsec != 0",
            "original_max_gap_gate":"jdelta>maxsep*3600"}
    (out.parent/"p6c-comp-trace-instrumentation.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))
def parse_profiles(p:Path):
    source={}
    for line in p.read_text().splitlines():
        if not line.startswith("   6201"):continue
        x=line.split()
        if len(x)!=14:raise ValueError("unexpected UP.DAT header")
        y,mo,d,h,s=map(int,x[7:12])
        t=datetime.datetime(y,mo,d,h,0,s,tzinfo=datetime.timezone.utc)
        if int(t.timestamp()) in source:raise ValueError("duplicate upper sounding")
        j=int(t.strftime("%j"))
        source[int(t.timestamp())]=y*100000+j*100+h
    if len(source)!=13:raise ValueError("expected 13 original sounding headers")
    return source
def compare(runtime:Path,joint:Path,up:Path,out:Path):
    trace=[]
    for line in runtime.read_text(errors="replace").splitlines():
        if "P6C_COMP_TRACE" not in line:continue
        parts=line.split("P6C_COMP_TRACE",1)[1].split()
        if len(parts)!=8:raise ValueError("original COMP trace width")
        trace.append(tuple(map(int,parts)))
    with joint.open(newline="") as f:
        records=list(csv.DictReader(f))
    if len(trace)!=143 or len(records)!=1287:
        raise ValueError(f"not 143 COMP states and 143x9 C++ hourly observations: {len(trace)}, {len(records)}")
    profiles=parse_profiles(up)
    stamps=list(profiles.keys())
    bad=[];turnovers=0;jorder_changes=0;last_order=1
    first=datetime.datetime(2022,1,1,tzinfo=datetime.timezone.utc)
    for n,state in enumerate(trace):
        now,secs,ntaa,ntbb,justa,jdelta,seca,secb=state
        hour=n+1
        instant=first+datetime.timedelta(hours=hour)
        actual_now=instant.year*100000+int(instant.strftime("%j"))*100+instant.hour
        record=records[n*9]
        aa_idx=int(record["aa_index"])
        bb_idx=int(record["bb_index"])
        expected_prior=profiles[stamps[aa_idx-1]]
        expected_follow=profiles[stamps[bb_idx-1]]
        # In original COMP AA and BB are *physical* buffers. JORDER JUSTA
        # indicates newest slot, so the chronological AA/BB labels switch.
        physical_aa=expected_prior if justa==1 else expected_follow
        physical_bb=expected_follow if justa==1 else expected_prior
        required={"nowtze":(now,actual_now),"nsecze":(secs,0),
                  "original_ntzaa":(ntaa,physical_aa),
                  "original_ntzbb":(ntbb,physical_bb),
                  "JUSTA_JORDER":(justa,int(record["jorder"])),
                  "original_gap_s":(jdelta,int(record["gap_s"])),
                  "original_jaasec":(seca,0),"original_jbbsec":(secb,0)}
        for name,(actual,expected) in required.items():
            if actual!=expected:
                bad.append({"hour":hour,"field":name,
                            "original":actual,"cpp_expected":expected})
        if justa!=last_order:jorder_changes+=1
        last_order=justa
        turnovers=max(turnovers,int(record["turnovers"]))
    report={"status":"PASS_ORIGINAL_COMP_RUNTIME_AA_BB" if not bad else "FAIL",
            "compared_original_hours":143,
            "physical_aa_bb_timestamps_checked":286,
            "source_time_and_sec_checks":143*3,
            "original_justa_jorder_switches":jorder_changes,
            "cpp_candidate_buffer_turnovers":turnovers,
            "scopes":[{"hours":h,"states":h} for h in [1,6,143]],
            "input_sha256":{"original_comp_runtime_log":sha(runtime),
                            "cpp_joint_csv":sha(joint),"up_dat":sha(up)},
            "errors":bad[:40],
            "scope":"Full original COMP runtime AA/BB timestamps/JUSTA/NSECDT/MAXSEP selected case; not full RDSN/CGAMMA/wind-field P6D parity"}
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))
    if bad:raise SystemExit(1)
def main():
    p=argparse.ArgumentParser()
    subs=p.add_subparsers(dest="cmd",required=True)
    i=subs.add_parser("instrument")
    i.add_argument("--vendor",required=True,type=Path)
    i.add_argument("--out",required=True,type=Path)
    c=subs.add_parser("compare")
    for name in ("runtime","joint","up","out"):
        c.add_argument("--"+name,required=True,type=Path)
    a=p.parse_args()
    if a.cmd=="instrument":instrument(a.vendor,a.out)
    else:compare(a.runtime,a.joint,a.up,a.out)
if __name__=="__main__":main()
