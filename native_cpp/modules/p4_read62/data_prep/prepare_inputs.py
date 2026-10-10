#!/usr/bin/env python3
"""Scoped P4 data preparation. Never executes the legacy READ62 binary."""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[4]
MODULE = "native_cpp.modules.p4_read62.data_prep"


def commands(year: int, pstop: float, station_id: str | None, skip_download: bool) -> list[list[str]]:
    if not 1900 <= year <= 2100:
        raise ValueError("year outside supported input interval")
    if not (0.0 < pstop <= 1500.0):
        raise ValueError("PSTOP must be within (0,1500] hPa")
    if station_id is not None and not station_id.strip():
        raise ValueError("empty station ID")
    steps: list[list[str]] = []
    if not skip_download:
        selection = ["--station-id", station_id] if station_id else ["--auto-select-all"]
        steps.append([sys.executable, "-m", MODULE + ".download_igra",
                      "--year", str(year), *selection, "--yes"])
    steps.append([sys.executable, "-m", MODULE + ".parse_igra_to_csv",
                  "--year", str(year), "--pstop", str(pstop), "--yes"])
    steps.append([sys.executable, "-m", MODULE + ".read", "--year", str(year)])
    return steps


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepare IGRA -> CSV -> FSL for P4 (no READ62 Fortran execution)."
    )
    parser.add_argument("--year", type=int, default=2022)
    parser.add_argument("--pstop", type=float, default=500.0)
    parser.add_argument("--station-id", help="Full IGRA station ID; otherwise select eligible stations")
    parser.add_argument("--skip-download", action="store_true")
    parser.add_argument("--execute", action="store_true",
                        help="Opt in to download and write project/current outputs; default prints commands only")
    args = parser.parse_args()
    try:
        steps = commands(args.year, args.pstop, args.station_id, args.skip_download)
    except ValueError as exc:
        parser.error(str(exc))
    if args.execute:
        from engine.core.paths import current_project_path, engine_path
        root = current_project_path()
        required = [root / "grid" / "center_location",
                    root / "grid" / "grid.cmn",
                    engine_path("resources", "igra", "igra2-station-list.txt")]
        if args.skip_download:
            required.append(root / "met" / "upperair" / str(args.year) / "upperair_station_used_list.txt")
        missing = [str(x) for x in required if not x.is_file()]
        if missing:
            raise SystemExit("P4 preflight failed: missing case/resource inputs:\n" + "\n".join(missing))
    for cmd in steps:
        print(" ".join(cmd), flush=True)
        if args.execute:
            subprocess.run(cmd, cwd=REPO_ROOT, check=True)
    if not args.execute:
        print("DRY RUN: no network access, files or legacy Fortran execution")
    else:
        print("P4 input preparation complete; FSL input produced, UP.DAT reference not generated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
