#!/usr/bin/env python3
"""Build ORIGINAL unmodified CALPUFF 7.2.1 in a disposable isolated directory.

Compiler build success only; this script neither runs a study case nor
establishes original/C++ physics parity. Original engine/vendor/src unchanged.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

REPO = Path(__file__).resolve().parents[4]
VENDOR = REPO / "engine/vendor/src/CALPUFF_v7.2.1_L150618"
SOURCE = ("modules.for", "calpuff.for")
FLAGS = ("-O0", "-std=legacy", "-fno-automatic", "-fno-align-commons",
         "-fallow-argument-mismatch", "-ffixed-line-length-none")

def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def build(output: Path, source: Path, compiler: str) -> None:
    if not source.is_dir():
        raise ValueError(f"Missing original source tree: {source}")
    for name in SOURCE:
        if not (source / name).is_file():
            raise ValueError(f"Missing original source: {name}")
    output = output.resolve()
    source = source.resolve()
    if output == source or source in output.parents:
        raise ValueError("Build output must not be inside original vendor source")
    output.mkdir(parents=True, exist_ok=True)
    include = output / "casefold_includes"
    include.mkdir(exist_ok=True)
    # FORTRAN INCLUDE names use mixed/upper case and historical case-insensitive
    # compiler conventions. No source file modification; link lower-case aliases.
    for path in sorted(source.iterdir()):
        if path.is_file():
            alias = include / path.name.lower()
            if alias.exists() or alias.is_symlink():
                if alias.resolve() != path.resolve():
                    raise ValueError(f"Case-fold filename collision: {alias}")
            else:
                alias.symlink_to(path)
    binary = output / "calpuff-original.x"
    command = [compiler, *FLAGS, "-I", str(include),
               *(str(source / item) for item in SOURCE),
               "-o", str(binary)]
    version = subprocess.run([compiler, "--version"], capture_output=True,
                             text=True, check=True).stdout.splitlines()[0]
    proc = subprocess.run(command, cwd=output, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output / "build.log").write_text(proc.stdout)
    manifest = {
        "stage": "P7A_UNMODIFIED_FORTRAN_COMPILER_REFERENCE",
        "authority": "CALPUFF 7.2.1 L150618",
        "source": str(source),
        "compiler": compiler,
        "compiler_version": version,
        "compiler_arguments": command,
        "sources_sha256": {name: sha256(source / name) for name in SOURCE},
        "returncode": proc.returncode,
        "original_run_executed": False,
        "numerical_parity": "NOT_TESTED",
    }
    if proc.returncode == 0 and binary.is_file():
        manifest["binary_sha256"] = sha256(binary)
    (output / "build-manifest.json").write_text(json.dumps(manifest, indent=2)+"\n")
    if proc.returncode:
        raise RuntimeError("Original CALPUFF compilation failed: see build.log")
    print("P7A_ORIGINAL_FORTRAN_COMPILE=PASS")
    print("P7A_CALPUFF_2022_REFERENCE_RUN=PENDING")

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--source",type=Path,default=VENDOR)
    p.add_argument("--compiler",default="gfortran")
    a=p.parse_args()
    build(a.output,a.source,a.compiler)
