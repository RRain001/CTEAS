#!/usr/bin/env python3
"""Retrieve source-locked historic 2023 met/control/concentration blobs.

Fetches only when absent or SHA1 differs, using authenticated GitHub REST.
Never writes to engine/vendor, current case, or a checked-in output file.
"""
from __future__ import annotations
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
from urllib.request import Request, urlopen
from legacy_2023_case import CASE, PINS, digest

def get(repo: str, token: str, sha: str) -> bytes:
    request=Request(f"https://api.github.com/repos/{repo}/git/blobs/{sha}",
                    headers={"Authorization":"Bearer "+token,
                    "Accept":"application/vnd.github+json",
                    "User-Agent":"CTEAS-P7-historical-2023-original-blob",
                    "X-GitHub-Api-Version":"2022-11-28"})
    with urlopen(request,timeout=180) as response:
        obj=json.load(response)
    if obj.get("encoding")!="base64":
        raise ValueError("GitHub large original case blob not provided as base64")
    raw=base64.b64decode(obj["content"])
    got=hashlib.sha1(f"blob {len(raw)}\0".encode()+raw).hexdigest()
    if got!=sha:
        raise ValueError(f"GitHub historical source SHA mismatch: {sha} != {got}")
    return raw

def fetch_case(root: Path, repo: str, token: str) -> None:
    for rel, expected in PINS.items():
        path=root/CASE/rel
        if path.exists():
            have=digest(path)
            if have["git_blob_sha1"]==expected:
                print("P7_2023_GIT_PIN=PASS cached",rel,have["bytes"])
                continue
            raise ValueError(f"Refusing locally changed historical input: {path}")
        raw=get(repo,token,expected)
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes(raw)
        print("P7_2023_GIT_PIN=PASS fetched",rel,len(raw))
    print("P7_HISTORICAL_SOURCE_LOCKED=PASS")

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--root",type=Path,required=True)
    args=parser.parse_args()
    token=os.environ.get("GH_TOKEN")
    if not token:raise SystemExit("Missing GH_TOKEN: read-only GitHub Actions token required")
    fetch_case(args.root,os.environ["GITHUB_REPOSITORY"],token)
