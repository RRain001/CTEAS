#!/usr/bin/env python3
"""Scoped P5 data preparation. Does not run original SMERGE or publish SURF.DAT."""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[4]
MODULE = "native_cpp.modules.p5_smerge.data_prep"


def commands(skip_download: bool, write_legacy_control: bool) -> list[list[str]]:
    stages = ([] if skip_download else ["download_isd_by_grid"]) + [
        "isd_to_smerge_and_fill",
        "clean_merge_filled_by_research_time",
        "convert_cleaned_to_smerge_ready",
    ]
    if write_legacy_control:
        stages.append("generate_smerge_input_sections")
    return [[sys.executable, "-m", MODULE + "." + step] for step in stages]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepare NOAA Global Hourly -> SMERGE GENERIC CSV; no Fortran execution."
    )
    parser.add_argument("--skip-download", action="store_true")
    parser.add_argument("--write-legacy-control", action="store_true",
                        help="Opt in to updating project/current/run/smerge/smerge.inp")
    parser.add_argument("--expected-year", type=int, default=2022,
                        help="Fail closed if case research_time does not match this year")
    parser.add_argument("--stack-lat", type=float, help="stack latitude")
    parser.add_argument("--stack-lon", type=float, help="stack longitude")
    parser.add_argument("--execute", action="store_true",
                        help="Opt in to writing project/current outputs; default prints plan only")
    args = parser.parse_args()
    steps = commands(args.skip_download, args.write_legacy_control)
    if not args.skip_download and (args.stack_lat is not None or args.stack_lon is not None):
        if args.stack_lat is None or args.stack_lon is None:
            parser.error("both stack coordinates required")
        steps[0].extend(["--stack-lat", str(args.stack_lat), "--stack-lon", str(args.stack_lon)])
    if args.execute:
        from engine.core.paths import current_project_path, engine_path
        root = current_project_path()
        required = [root / "grid" / "center_location",
                    root / "grid" / "grid.cmn",
                    root / "grid" / "research_time",
                    engine_path("resources", "isd", "isd-history.txt")]
        if args.skip_download:
            required.append(root / "met" / "surface" / "isd_raw")
        if args.write_legacy_control:
            required.append(root / "run" / "smerge" / "smerge.inp")
        missing = [str(x) for x in required if not x.exists()]
        if missing:
            raise SystemExit("P5 preflight failed: missing case/resource inputs:\n" + "\n".join(missing))
        times = (root / "grid" / "research_time").read_text(encoding="utf-8", errors="ignore")
        begin = re.search(r"BEGIN\s*:\s*(\d{8})-\d{6}", times)
        end = re.search(r"END\s*:\s*(\d{8})-\d{6}", times)
        if not begin or not end or int(begin.group(1)[:4]) != args.expected_year or int(end.group(1)[:4]) != args.expected_year:
            raise SystemExit("P5 research_time mismatch: expected entire case in " + str(args.expected_year))
    if args.execute and args.skip_download:
        import json
        from engine.core.paths import current_project_path
        root = current_project_path() / "met" / "surface" / "isd_raw"
        source = root / f"station_selection_{args.expected_year}.json"
        if not source.is_file():
            raise SystemExit(f"cannot reuse data without station manifest: {source}")
        selected = {s["sid"] for s in json.loads(source.read_text())["stations"]}
        actual = {p.name.split("-")[0] for p in root.glob(f"*-{args.expected_year}.csv")}
        if selected != actual:
            raise SystemExit(f"stale or unexpected NOAA raw files: {actual - selected}, missing: {selected - actual}")
    for cmd in steps:
        print(" ".join(cmd), flush=True)
        if args.execute:
            subprocess.run(cmd, cwd=REPO_ROOT, check=True)
    if not args.execute:
        print("DRY RUN: no network, files, SMERGE binary or SURF.DAT publication")
    else:
        print("P5 GENERIC-ready preparation complete; original SMERGE reference not generated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
