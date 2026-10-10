#!/usr/bin/env python3
"""Audit a REAL P5 Scheme-C GENERIC sample against original-hour wind audit.

Requires no network and never writes to the original input directory.
"""
from __future__ import annotations

import argparse
import csv
import json
from datetime import datetime
from pathlib import Path


def run(case_root: Path, expected_hours: int = 144) -> dict:
    base = case_root / "met" / "surface"
    selected = sorted((base / "isd_raw").glob("station_selection_*.json"))
    if len(selected) != 1:
        raise ValueError(f"expected one case-year station selection manifest, got {selected}")
    manifest = json.loads(selected[0].read_text(encoding="utf-8"))
    if len(manifest.get("stations", [])) != 1:
        raise ValueError("expected a single selected virtual surface station")
    station = manifest["stations"][0]
    if not station["relocated"]:
        raise ValueError("this integration capture is explicitly a virtual-station scenario")
    ready = base / "isd2smerge_ready" / (station["sid"][:6] + "-" + station["sid"][6:] + ".csv")
    wind_audit = base / "isd2smerge_and_fill" / (
        station["sid"][:6] + "-" + station["sid"][6:] + "-" +
        str(manifest["year"]) + "_filled_wind_audit.csv"
    )
    summary = wind_audit.with_suffix(".json")
    if not ready.is_file() or not wind_audit.is_file() or not summary.is_file():
        raise FileNotFoundError(f"missing READY or wind audit: {ready} / {wind_audit}")
    with ready.open(newline="", encoding="utf-8") as f:
        header = [f.readline().strip() for _ in range(3)]
        data = list(csv.reader(f))
    if header[2] != "Month,Day,Year,Hour,DegC,mm,mb,%,deg,ms-1,tenths,hundreds_of_feet":
        raise ValueError(f"GENERIC columns changed: {header[2]}")
    if header[1].split(",")[3].strip() != str(station["ifstn"]):
        raise ValueError("GENERIC numeric station ID differs from selection manifest")
    if len(data) != expected_hours:
        raise ValueError(f"GENERIC rows {len(data)} != expected {expected_hours}")

    with wind_audit.open(newline="", encoding="utf-8") as f:
        audits = {row["at_utc"]: row for row in csv.DictReader(f)}
    policy = json.loads(summary.read_text(encoding="utf-8"))
    if policy.get("wind_policy") != "P5_C_SOURCE_WIND_V1":
        raise ValueError("not a Scheme-C artifact")
    if policy.get("calm_limit_usage") != "LOW_SPEED_DIAGNOSTIC_ONLY":
        raise ValueError("0.5 m/s used for more than low-speed diagnosis")

    zero = low_nonzero = mismatched = 0
    for row in data:
        if len(row) != 12:
            raise ValueError(f"bad GENERIC field count: {row}")
        dt = datetime(int(row[2]), int(row[0]), int(row[1]), int(row[3]) // 100)
        time = dt.strftime("%Y-%m-%dT%H:%M:%SZ")
        record = audits.get(time)
        if record is None:
            raise ValueError(f"wind audit is missing hour {time}")
        speed = float(row[9])
        expected = float(record["wind_speed_generic_m_s"])
        filled = float(record["wind_speed_filled_m_s"])
        old_alias = float(record["wind_speed_calculation_m_s"])
        if abs(speed - expected) > 0.0005 or abs(expected - filled) > 0.0005 or abs(old_alias - expected) > 0.0005:
            mismatched += 1
        if speed == 0.0:
            zero += 1
            if float(row[8]) != 0:
                raise ValueError("exact zero-speed hour must use GENERIC undefined-direction convention WD=0")
            if record["is_exact_zero_speed"] != "True":
                raise ValueError("audit misclassified zero-speed hour")
        if 0 < speed < 0.5:
            low_nonzero += 1
        if (speed < 0.5) != (record["is_calm"] == "True"):
            raise ValueError(f"low-speed diagnostic flag mismatch at {time}")

    result = {
        "policy": "P5_C_SOURCE_WIND_V1",
        "station": station["sid"],
        "processed_hours": len(data),
        "matched_hours": len(data) - mismatched,
        "zero_speed_hours": zero,
        "nonzero_below_half_mps_hours": low_nonzero,
        "mismatched_speed_hours": mismatched,
        "original_calmet_calm_handling": policy.get("calmet_calm_handling"),
    }
    if mismatched:
        raise ValueError(f"Scheme C speed altered in {mismatched} hours")
    return result


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--case-root", type=Path, default=Path("project/current"))
    ap.add_argument("--expected-hours", type=int, default=144)
    ap.add_argument("--report", type=Path)
    args = ap.parse_args()
    result = run(args.case_root, args.expected_hours)
    print("P5_SCHEME_C_REAL_WIND_EQUIVALENCE=PASS", json.dumps(result, sort_keys=True))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
