#!/usr/bin/env python3
"""P6D D-2 fixed-SHA stage bisection against six original DIAGNO snapshots.

Diagnostics only. The snapshots are write-only traces from original Fortran, not
the synthetic WIND1/INTER2 fixture reference. A green exit means the evidence
was decoded and compared correctly, never that the wind physics has passed.
"""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import math
import struct
from pathlib import Path

EXPECTED_SHA = "972af841552f7a0eda6b02b221b6b2a808c61806054e634c617425d9ca9babdf"
EXPECTED_HOURS = (1, 2, 3, 81, 82, 83)
NX, NY, NZ = 50, 50, 9
N3 = NX * NY * NZ
NF = NX * NY * (NZ + 1)
FIELD_COUNTS = {7: (N3, N3), 8: (N3, N3), 9: (N3, N3),
                10: (N3, NF, N3, N3)}
FIELD_NAMES = {7: ("USLOPE", "VSLOPE"), 8: ("UG", "VG"),
               9: ("UG", "VG"), 10: ("DIV", "W", "U", "V")}
# The before-slope product is only a proxy for kpt8 when FRADJ has not been
# supplied. It must NOT be called original stage parity.
STAGES = (
    ("pre_slope_proxy", 8, "wind1", ("U", "V")),
    ("post_slope", 9, "slope_corrected", ("U", "V")),
    ("final_wind", 10, "blend", ("U", "V")),
    ("final_w_faces", 10, "w_face", ("W",)),
)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for part in iter(lambda: f.read(1024 * 1024), b""):
            h.update(part)
    return h.hexdigest()


def label_from_begin(packed: int, seconds: int) -> str:
    year = packed // 100000
    julian = (packed // 100) % 1000
    hour = packed % 100
    if year != 2022 or not (1 <= julian <= 365) or not (0 <= hour <= 23):
        raise ValueError(f"Unexpected packed original time: {packed}")
    if seconds != 0:
        raise ValueError(f"Unexpected seconds: {seconds}")
    begin = dt.datetime(year, 1, 1) + dt.timedelta(days=julian - 1, hours=hour)
    return (begin + dt.timedelta(hours=1)).strftime("%Y-%m-%d_%H")


def load_snapshots(path: Path):
    if sha256(path) != EXPECTED_SHA:
        raise ValueError("Original trace SHA256 mismatch; refusing comparison")
    buf = path.read_bytes()
    offset = 0
    data = {}
    hour_seen = []
    for hour_index in EXPECTED_HOURS:
        stage_data = {}
        label = None
        for expected_kpt in (7, 8, 9, 10):
            if offset + 32 > len(buf):
                raise ValueError("Truncated DGM header")
            header = struct.unpack_from("<8i", buf, offset)
            offset += 32
            kpt, packed, seconds, i0, j0, nx, ny, nz = header
            if (kpt, nx, ny, nz) != (expected_kpt, NX, NY, NZ):
                raise ValueError(f"Wrong DGM header: {header}")
            this_label = label_from_begin(packed, seconds)
            if label is not None and label != this_label:
                raise ValueError("Mismatched hourly clocks in original DGM trace")
            label = this_label
            sample = {}
            for name, count in zip(FIELD_NAMES[kpt], FIELD_COUNTS[kpt]):
                size = 4 * count
                if offset + size > len(buf):
                    raise ValueError(f"Truncated {name} at hour {hour_index}")
                sample[name] = struct.unpack_from(f"<{count}f", buf, offset)
                offset += size
            stage_data[kpt] = sample
        data[label] = stage_data
        hour_seen.append({"hour_index": hour_index, "label": label})
    if offset != len(buf) or len(data) != len(EXPECTED_HOURS):
        raise ValueError("Trailing data, duplicated labels or missing DGM records")
    return data, hour_seen


def candidate(path: Path, n: int):
    if not path.is_file() or path.stat().st_size != 4 * n:
        raise ValueError(f"Missing/wrong-size C++ plane: {path} (want {4*n} bytes)")
    values = struct.unpack(f"<{n}f", path.read_bytes())
    if not all(math.isfinite(x) and abs(x) < 9999 for x in values):
        raise ValueError(f"Invalid candidate values: {path}")
    return values


def compare(ref, actual, nlevels: int):
    nxy = NX * NY
    rows = []
    for lev in range(nlevels):
        a = ref[lev * nxy:(lev + 1) * nxy]
        b = actual[lev * nxy:(lev + 1) * nxy]
        if len(a) != nxy or len(b) != nxy:
            raise ValueError("Wrong candidate/reference levels")
        diffs = [b[i] - a[i] for i in range(nxy)]
        over = sum(abs(d) > 1e-4 + 1e-5 * abs(a[i]) for i, d in enumerate(diffs))
        rows.append({
            "level_one_based": lev + 1,
            "original_spread": max(a) - min(a),
            "candidate_spread": max(b) - min(b),
            "maximum_absolute_error": max(map(abs, diffs)),
            "mean_signed_error": sum(diffs) / nxy,
            "rms_error": math.sqrt(sum(x * x for x in diffs) / nxy),
            "exceedances_at_frozen_wind_tolerance": over,
            "denominator": nxy,
            "original_center_26_26": a[25 * NX + 25],
            "candidate_center_26_26": b[25 * NX + 25],
        })
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--original", required=True, type=Path)
    parser.add_argument("--product", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    data, hours = load_snapshots(args.original)
    comparisons = {}
    for label, state in data.items():
        comparison = {}
        for stage, kpt, prefix, fields in STAGES:
            section = {}
            for field in fields:
                nlevels = NZ + 1 if field == "W" else NZ
                ref = state[kpt][field]
                if prefix == "w_face":
                    name = f"w_face_{label}.f32"
                else:
                    name = f"{prefix}_{field.lower()}_{label}.f32"
                actual = candidate(args.product / name, NX * NY * nlevels)
                section[field] = compare(ref, actual, nlevels)
            comparison[stage] = section
        comparisons[label] = comparison
    # Explicitly compare *corresponding* original stages; do not compare original
    # pre-SLOPE UG to post-DIVCEL blend. The first stage is labelled proxy because
    # the producer may not have a complete original FRADJ input.
    summary = {}
    for stage, _, _, fields in STAGES:
        by_field = {}
        for field in fields:
            records = [entry for label in comparisons
                       for entry in comparisons[label][stage][field]]
            by_field[field] = {
                "maximum_absolute_error": max(x["maximum_absolute_error"] for x in records),
                "total_exceedances": sum(x["exceedances_at_frozen_wind_tolerance"] for x in records),
                "compared_values": sum(x["denominator"] for x in records),
                "maximum_original_spread": max(x["original_spread"] for x in records),
                "maximum_candidate_spread": max(x["candidate_spread"] for x in records),
            }
        summary[stage] = by_field
    output = {
        "status": "DIAGNOSTIC_COMPLETED_NOT_FULL_P6D_PASS",
        "purpose": "Locate earliest apparent wind-field deviation without crossing stages",
        "reference": "original CALMET write-only DGM kpt 8/9/10, six frozen hours",
        "reference_sha256": EXPECTED_SHA,
        "shape": {"nx": NX, "ny": NY, "nz": NZ},
        "tolerance": "abs(diff) <= 1e-4 + 1e-5*abs(original), m/s",
        "cautions": [
            "pre_slope_proxy compares C++ WIND1 to original pre-SLOPE UG/VG; FRADJ may lie between them",
            "post_slope compares original post-SLOPE UG/VG to C++ slope_corrected",
            "final_wind compares original post-DIVCEL U/V to C++ blend",
            "final_w_faces compares original post-DIVCEL W to C++ w_face",
            "C++ producer without ZII/EL/GAMMA inputs is an incomplete forcing configuration",
            "success means a reproducible diagnosis; numerical exceedances remain failures for P6D acceptance",
            "only six frozen hours are compared; the final 143-hour acceptance is a separate gate",
        ],
        "hours": hours,
        "summary": summary,
        "results": comparisons,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    print("P6D_D2_STAGE_BISECTION=DIAGNOSTIC_COMPLETED_NOT_ACCEPTED")
    print(json.dumps({"hours": hours, "summary": summary}, indent=2))


if __name__ == "__main__":
    main()
