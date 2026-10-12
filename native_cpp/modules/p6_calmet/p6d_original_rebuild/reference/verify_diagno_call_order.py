#!/usr/bin/env python3
"""Prevent noncommutative FRADJ/SLOPE order regression in the real P6D producer.

This is a *source-order guard*, NOT a physical parity proof. The latter is
established by original-Fortran-vs-C++ field comparisons separately.
"""
from __future__ import annotations
import argparse
import hashlib
import re
from pathlib import Path

ORIGINAL="437f43bb7f0e51a1919e56664663e1a046986ed0"


def read_original(f: Path):
    raw=f.read_bytes()
    digest=hashlib.sha1(f"blob {len(raw)}\0".encode()+raw).hexdigest()
    if digest!=ORIGINAL:raise RuntimeError(f"CALMET original blob changed: {digest}")
    return raw.decode(errors="replace").splitlines()


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--vendor",type=Path,required=True)
    parser.add_argument("--producer",type=Path,required=True)
    a=parser.parse_args()
    lines=read_original(a.vendor)
    fr="IF (IFRADJ .EQ. 1) CALL FRADJ"
    sl="if (ISLOPE.EQ.1)CALL SLOPE"
    if fr not in lines[5642] or sl not in lines[5721]:
        raise RuntimeError("original DIAGNO FRADJ/SLOPE line anchors changed")
    producer=a.producer.read_text()
    # Match actual calls, not the text of comment explanations.
    match_f=re.findall(r"\bfroude_adjust\s*\(",producer)
    match_s=re.findall(r"\bcompute_original_slope_scoped\s*\(",producer)
    if len(match_f)!=1 or len(match_s)!=1:
        raise RuntimeError("P6D diagnostics expect exactly one FRADJ and SLOPE call")
    f=producer.index("froude_adjust(")
    s=producer.index("compute_original_slope_scoped(")
    blend=producer.index("original_inter2_selected(",s)
    if not (0<f<s<blend):
        raise RuntimeError("original FRADJ -> SLOPE -> INTER2 sequence violated")
    print("PASS_P6D_SOURCE_ORDER_FRADJ_BEFORE_SLOPE_BEFORE_INTER2")
    print("original CALMET: FRADJ L5643 then SLOPE L5722 then INTER2 L6055")


if __name__=="__main__":
    main()
