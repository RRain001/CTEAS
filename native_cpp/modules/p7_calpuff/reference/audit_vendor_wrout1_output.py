#!/usr/bin/env python3
"""Audit frozen original CALPUFF WROUT1/PG-AVET output, not fake parity.

For the frozen 2022 143-hour MWET=0 deck, PG AVET equals PGTIME (60min);
PG sigma-y adjustment is 1. That does NOT remove the need to reproduce the
143 hourly receptor accumulation and the WROUT1 headers/record order.
"""
from __future__ import annotations

import argparse
import collections
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

P7_FROZEN_CON_SHA = "0d51bdb68ff1a5798048bc41067107d3ffe10dc6222f25336df856bcbf48ba10"
ORIGINAL_RECORDS = 3655
DATA_RECORDS = 143*3
NONDATA_RECORDS = ORIGINAL_RECORDS-DATA_RECORDS


def one(deck: str, key: str) -> float:
    matches=re.findall(r"!\s*"+re.escape(key)+r"\s*=\s*([^!\n\r]+)!",deck,re.I)
    if len(matches)!=1:
        raise ValueError(f"control {key}: active value count={len(matches)}")
    return float(matches[0].strip().replace("D","E").replace("d","e"))


def audit(control: Path, original: Path, report: Path | None) -> dict:
    deck=control.read_text(encoding="latin-1")
    if int(one(deck,"MWET"))!=0 or int(one(deck,"METFM"))!=1:
        raise ValueError("selected dry original 2022 output only")
    avet=one(deck,"AVET")
    pgtime=one(deck,"PGTIME")
    if (avet,pgtime)!=(60.0,60.0):
        raise ValueError("PG averaging case-specific branch changed")
    factor=(avet/pgtime)**0.2
    if factor!=1.0:
        raise AssertionError("AVET is not neutral as presumed")
    data=original.read_bytes()
    if hashlib.sha256(data).hexdigest()!=P7_FROZEN_CON_SHA:
        raise AssertionError("frozen original puff.con SHA256 drift")
    hist=collections.Counter()
    entries=[]
    pos=0
    while pos<len(data):
        if pos+8>len(data):
            raise ValueError("truncated original Fortran sequential record marker")
        size=struct.unpack_from("<i",data,pos)[0]
        if size<0 or size>len(data)-pos-8:
            raise ValueError(f"invalid original record length {size} at {pos}")
        end=pos+4+size
        suffix=struct.unpack_from("<i",data,end)[0]
        if suffix!=size:
            raise ValueError(f"Fortran framing mismatch at {pos}: {size}/{suffix}")
        hist[size]+=1
        if len(entries)<12:
            entries.append({"ordinal":len(entries)+1,"byte_offset":pos,
                            "payload_bytes":size,
                            "first_16_hex":data[pos+4:pos+4+min(size,16)].hex()})
        pos=end+4
    if pos!=len(data) or sum(hist.values())!=ORIGINAL_RECORDS:
        raise AssertionError("frozen original Fortran record count changed")
    out={
        "status":"ORIGINAL_WROUT1_OUTPUT_REFERENCE_AUDITED",
        "control_scope":"Hefei 2022 METFM=1 MWET=0 143h",
        "original_con_sha256":P7_FROZEN_CON_SHA,
        "original_con_bytes":len(data),
        "original_sequential_records":ORIGINAL_RECORDS,
        "original_143_hour_data_records_from_p7g_structure_parity":DATA_RECORDS,
        "original_nondata_records":NONDATA_RECORDS,
        "current_cpp_minimal_writer_nondata_records":2,
        "missing_vendor_WROUT1_nondata_records":NONDATA_RECORDS-2,
        "original_record_lengths":{str(k):v for k,v in sorted(hist.items())},
        "original_first_records":entries,
        "avet_minutes":avet,"pgtime_minutes":pgtime,
        "original_PG_sigma_y_averaging_factor":factor,
        "avenue":"WROUT1 header records and whole-case concentration/period writer must be ported; AVET=PGTIME does not waive them",
        "native_header_numerical_parity":"NOT_YET_IMPLEMENTED",
        "native_143h_concentration_parity":"NOT_YET_IMPLEMENTED",
    }
    if report:
        report.parent.mkdir(parents=True,exist_ok=True)
        report.write_text(json.dumps(out,indent=2)+"\n")
    print("P7_ORIGINAL_WROUT1_AUDIT=PASS "
          f"RECORDS={ORIGINAL_RECORDS} DATA={DATA_RECORDS} "
          f"NONDATA={NONDATA_RECORDS} MISSING_CPP_NONDATA={NONDATA_RECORDS-2}")
    print("P7_ORIGINAL_PG_AVET=PASS AVET=60 PGTIME=60 AVERAGING_FACTOR=1")
    print("P7_CPP_WROUT1_143H_CONCENTRATION=NOT_YET_IMPLEMENTED")
    return out


def main() -> int:
    p=argparse.ArgumentParser()
    p.add_argument("--control",type=Path,required=True)
    p.add_argument("--original-con",type=Path,required=True)
    p.add_argument("--report",type=Path)
    args=p.parse_args()
    try:
        audit(args.control,args.original_con,args.report)
        return 0
    except (OSError,ValueError,AssertionError,struct.error) as e:
        print("P7_ORIGINAL_WROUT1_AUDIT=FAIL "+str(e),file=sys.stderr)
        return 1

if __name__=="__main__":
    raise SystemExit(main())
