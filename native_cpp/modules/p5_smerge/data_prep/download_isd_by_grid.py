#!/usr/bin/env python3
"""
Download ISD global-hourly station CSVs by study-area bounds.

Workflow:
1) Read center point from center_location (longitude/latitude).
2) Read grid size from grid.cmn (DGRIDKM, NX, NY).
3) Build lat/lon bounding box from center and grid span.
4) Filter stations in isd_station_list.txt that:
   - are inside bbox
   - have data in target year based on BEGIN/END.
5) Download station files from:
   https://www.ncei.noaa.gov/data/global-hourly/access/{year}/{station_id}.csv
"""

from __future__ import annotations

import argparse
import csv
import math
import re
from datetime import datetime, timedelta
from dataclasses import dataclass
from pathlib import Path
from urllib import error, request

from engine.core.paths import current_project_path, engine_path

CASE_ROOT = current_project_path()

DEFAULT_CENTER = CASE_ROOT / "grid" / "center_location"
DEFAULT_GRID = CASE_ROOT / "grid" / "grid.cmn"
DEFAULT_HISTORY = engine_path("resources", "isd", "isd-history.txt")
DEFAULT_RESEARCH_TIME = CASE_ROOT / "grid" / "research_time"
DEFAULT_OUT_DIR = CASE_ROOT / "met" / "surface" / "isd_raw"
BASE_URL = "https://noaa-global-hourly-pds.s3.amazonaws.com"


@dataclass
class Station:
    sid: str
    lat: float
    lon: float
    elev: float
    name: str
    country: str
    begin: str
    end: str


def parse_center(path: Path) -> tuple[float, float]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    m_lon = re.search(r"longitude\s*:\s*([+-]?\d+(?:\.\d+)?)", text, flags=re.IGNORECASE)
    m_lat = re.search(r"latitude\s*:\s*([+-]?\d+(?:\.\d+)?)", text, flags=re.IGNORECASE)
    if not m_lon or not m_lat:
        raise ValueError(f"Cannot parse longitude/latitude from: {path}")
    return float(m_lon.group(1)), float(m_lat.group(1))


def parse_grid(path: Path) -> tuple[float, int, int]:
    text = path.read_text(encoding="utf-8", errors="ignore")

    def find_num(key: str) -> str:
        m = re.search(rf"!\s*{re.escape(key)}\s*=\s*([^\s!]+)", text, flags=re.IGNORECASE)
        if not m:
            raise ValueError(f"Cannot find {key} in {path}")
        return m.group(1)

    dgrid_km = float(find_num("DGRIDKM"))
    nx = int(float(find_num("NX")))
    ny = int(float(find_num("NY")))
    return dgrid_km, nx, ny


def parse_history_txt(path: Path) -> list[Station]:
    stations: list[Station] = []
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    data_started = False
    for line in lines:
        s = line.strip()
        if not data_started:
            if s.startswith("USAF") and "WBAN" in s and "BEGIN" in s and "END" in s:
                data_started = True
            continue
        if not s:
            continue
        try:
            # NOAA ISD history fixed-width first (more reliable for station names)
            usaf = line[0:6].strip()
            wban = line[7:12].strip()
            name = line[13:43].strip()
            country = line[43:46].strip() or "UNK"
            lat = float(line[57:64].strip())
            lon = float(line[65:73].strip())
            elev = float(line[74:81].strip())
            begin = line[82:90].strip()
            end = line[91:99].strip()
            if not usaf or not wban or not begin or not end:
                raise ValueError("fixed-width parse fallback")

            sid = f"{usaf}{wban}"
            stations.append(
                Station(
                    sid=sid,
                    lat=lat,
                    lon=lon,
                    elev=elev,
                    name=name,
                    country=country,
                    begin=begin,
                    end=end,
                )
            )
        except Exception:
            # fallback split parse
            parts = s.split()
            if len(parts) < 9:
                continue
            try:
                usaf = parts[0]
                wban = parts[1]
                sid = f"{usaf}{wban}"
                lat = float(parts[-5])
                lon = float(parts[-4])
                elev = float(parts[-3])
                begin = parts[-2]
                end = parts[-1]
                country = parts[3] if len(parts) > 4 else "UNK"
                name = " ".join(parts[2:-7]).strip() if len(parts) > 9 else ""
                stations.append(
                    Station(
                        sid=sid,
                        lat=lat,
                        lon=lon,
                        elev=elev,
                        name=name,
                        country=country,
                        begin=begin,
                        end=end,
                    )
                )
            except Exception:
                continue
    return stations


def parse_research_time(path: Path) -> tuple[datetime, datetime]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    m_begin = re.search(r"BEGIN\s*:\s*(\d{8}-\d{6})", text)
    m_end = re.search(r"END\s*:\s*(\d{8}-\d{6})", text)
    if not m_begin or not m_end:
        raise ValueError(f"Cannot parse BEGIN/END from: {path}")
    begin = datetime.strptime(m_begin.group(1), "%Y%m%d-%H%M%S")
    end = datetime.strptime(m_end.group(1), "%Y%m%d-%H%M%S")
    if end < begin:
        raise ValueError(f"END earlier than BEGIN in: {path}")
    return begin, end


def build_bbox(center_lon: float, center_lat: float, dgrid_km: float, nx: int, ny: int) -> tuple[float, float, float, float]:
    # Treat center_location as center of study area.
    half_w_km = dgrid_km * nx / 2.0
    half_h_km = dgrid_km * ny / 2.0

    dlat = half_h_km / 111.32
    cos_lat = math.cos(math.radians(center_lat))
    if abs(cos_lat) < 1e-8:
        dlon = 180.0
    else:
        dlon = half_w_km / (111.32 * cos_lat)

    min_lon = center_lon - dlon
    max_lon = center_lon + dlon
    min_lat = center_lat - dlat
    max_lat = center_lat + dlat
    return min_lon, max_lon, min_lat, max_lat


def covers_interval(begin: str, end: str, first: datetime, last: datetime) -> bool:
    """Station-history coverage for the exact requested period, inclusive."""
    if not (re.fullmatch(r"\d{8}", begin or "") and re.fullmatch(r"\d{8}", end or "")):
        return False
    return begin <= first.strftime("%Y%m%d") and end >= last.strftime("%Y%m%d")


def haversine_km(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    dl = math.radians(lat2 - lat1)
    do = math.radians(lon2 - lon1)
    a = math.sin(dl / 2) ** 2 + math.cos(math.radians(lat1)) * math.cos(
        math.radians(lat2)
    ) * math.sin(do / 2) ** 2
    return 6371.0088 * 2 * math.atan2(math.sqrt(a), math.sqrt(max(0, 1 - a)))


def read_stack_position(args: argparse.Namespace) -> tuple[float, float] | None:
    """Never silently equate model domain center with the release-stack location."""
    if (args.stack_lat is None) != (args.stack_lon is None):
        raise ValueError("both --stack-lat and --stack-lon are required")
    if args.stack_lat is not None:
        lat, lon = args.stack_lat, args.stack_lon
    elif args.stack_file and args.stack_file.is_file():
        lon, lat = parse_center(args.stack_file)
    else:
        return None
    if not (-90 <= lat <= 90 and -180 <= lon <= 180):
        raise ValueError("stack latitude/longitude outside valid range")
    return lat, lon


def inside_bbox(st: Station, min_lon: float, max_lon: float, min_lat: float, max_lat: float) -> bool:
    return min_lon <= st.lon <= max_lon and min_lat <= st.lat <= max_lat


def station_url(year: int, sid: str) -> str:
    return f"{BASE_URL}/{year}/{sid}.csv"


def expected_hours_in_year(year: int) -> int:
    leap = (year % 4 == 0 and year % 100 != 0) or (year % 400 == 0)
    return 8784 if leap else 8760


REQUIRED_NOAA_FIELDS = {"STATION", "DATE", "WND", "TMP", "DEW", "SLP"}


def original_coverage(csv_path: Path, year: int, station_id: str | None = None) -> tuple[int, int]:
    """Count unique UTC source hours BEFORE all filling, not the final 8760 rows."""
    times: set[datetime] = set()
    with csv_path.open("r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        if not reader.fieldnames or not REQUIRED_NOAA_FIELDS.issubset(reader.fieldnames):
            raise ValueError(f"NOAA schema mismatch: {reader.fieldnames}")
        for row in reader:
            if station_id is not None and (row.get("STATION") or "").strip() != station_id:
                raise ValueError(f"NOAA STATION mismatch expected {station_id}")
            raw = (row.get("DATE") or "").strip()
            if not raw:
                continue
            try:
                parsed = datetime.fromisoformat(raw.replace("Z", "+00:00"))
            except ValueError:
                continue
            # NOAA Global Hourly timestamps are UTC; normalize aware timestamps.
            if parsed.tzinfo is not None:
                from datetime import timezone
                parsed = parsed.astimezone(timezone.utc).replace(tzinfo=None)
            if parsed.year == year:
                times.add(parsed.replace(minute=0, second=0, microsecond=0))
    return len(times), expected_hours_in_year(year)


def validate_time_completeness(csv_path: Path, year: int, station_id: str | None = None) -> tuple[bool, str]:
    """Explicit >=80% annual *raw time* coverage. 2022: >=7008 of 8760."""
    try:
        found, expected = original_coverage(csv_path, year, station_id)
    except (OSError, ValueError) as exc:
        return False, str(exc)
    ratio = found / expected
    accepted = found >= math.ceil(expected * 0.8 - 1e-10)
    return accepted, f"source_unique_hours={found}/{expected}; coverage={ratio:.4%}; >=80%={'PASS' if accepted else 'FAIL'}"


def download_one(year: int, sid: str, out_dir: Path, timeout: int) -> tuple[bool, str, bool]:
    out_dir.mkdir(parents=True, exist_ok=True)
    out_file = out_dir / f"{sid}-{year}.csv"
    url = station_url(year, sid)
    if out_file.exists() and out_file.stat().st_size > 0:
        try:
            original_coverage(out_file, year, sid)  # validate all cached NOAA identity/schema
        except (OSError, ValueError) as exc:
            return False, f"invalid cached source {out_file}: {exc}", True
        return True, f"已存在, 跳过：{out_file}", True
    try:
        with request.urlopen(url, timeout=timeout) as resp:
            data = resp.read()
        # The former NCEI endpoint retired in July 2026 and can return
        # 200-OK HTML. Reject anything other than the expected station CSV.
        try:
            header = next(csv.reader([data.split(b"\n", 1)[0].decode("utf-8-sig")]))
        except (UnicodeDecodeError, csv.Error, StopIteration) as exc:
            raise ValueError("NOAA Global Hourly response is not CSV") from exc
        required = REQUIRED_NOAA_FIELDS
        if not required.issubset(header):
            raise ValueError(f"NOAA Global Hourly CSV headers absent: {header[:8]!r}")
        if len(data) < 256:
            raise ValueError("NOAA Global Hourly CSV response suspiciously short")
        out_file.write_bytes(data)
        try:
            original_coverage(out_file, year, sid)
        except (OSError, ValueError) as exc:
            out_file.unlink(missing_ok=True)
            raise ValueError(f"Downloaded NOAA CSV identity/schema mismatch: {exc}") from exc
        return True, str(out_file), False
    except error.HTTPError as e:
        return False, f"HTTP {e.code}", False
    except Exception as e:
        return False, str(e), False


def main() -> int:
    ap = argparse.ArgumentParser(description="P5 scoped NOAA source selection and download")
    ap.add_argument("--year", type=int, help="target calendar year, else research_time years")
    ap.add_argument("--center-file", type=Path, default=DEFAULT_CENTER)
    ap.add_argument("--grid-file", type=Path, default=DEFAULT_GRID)
    ap.add_argument("--research-time-file", type=Path, default=DEFAULT_RESEARCH_TIME)
    ap.add_argument("--history-file", type=Path, default=DEFAULT_HISTORY)
    ap.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    ap.add_argument("--stack-lat", type=float, default=None)
    ap.add_argument("--stack-lon", type=float, default=None)
    ap.add_argument("--stack-file", type=Path,
                    default=CASE_ROOT / "grid" / "stack_location")
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--retries", type=int, default=3)
    ap.add_argument("--max-stations", type=int, default=0,
                    help="legacy argument; only 0 is accepted (all in-domain stations)")
    args = ap.parse_args()
    if args.max_stations != 0:
        ap.error("--max-stations must be 0: never truncate in-domain stations")
    if args.year is not None and not 1901 <= args.year <= 2100:
        ap.error("invalid target year")
    if args.retries < 1:
        ap.error("--retries must be >=1")
    research_start, research_end = parse_research_time(args.research_time_file)
    if args.year is not None:
        years = [args.year]
    else:
        years = list(range(research_start.year, research_end.year + 1))
    center_lon, center_lat = parse_center(args.center_file)
    dgrid, nx, ny = parse_grid(args.grid_file)
    bbox = build_bbox(center_lon, center_lat, dgrid, nx, ny)
    stack = read_stack_position(args)
    stations = parse_history_txt(args.history_file)
    if not stations:
        raise SystemExit("NOAA history is empty or invalid")
    from .station_identity import numeric_station_ids
    import json

    args.out_dir.mkdir(parents=True, exist_ok=True)
    for year in years:
        first = max(research_start, datetime(year, 1, 1))
        last = min(research_end, datetime(year, 12, 31, 23, 59, 59))
        if first > last:
            if args.year is not None:
                first, last = datetime(year, 1, 1), datetime(year, 12, 31, 23, 59, 59)
            else:
                continue
        eligible = [st for st in stations if covers_interval(st.begin, st.end, first, last)]
        in_domain = sorted((st for st in eligible if inside_bbox(st, *bbox)), key=lambda st: st.sid)
        mode = "OBSERVED" if in_domain else "VIRTUAL_RELOCATED"
        if mode == "VIRTUAL_RELOCATED" and stack is None:
            raise SystemExit("No stations inside research domain: --stack-lat/--stack-lon "
                             "or grid/stack_location required for nearest-station fallback")
        if mode == "VIRTUAL_RELOCATED":
            ordered = sorted(eligible, key=lambda st: (
                haversine_km(stack[0], stack[1], st.lat, st.lon), st.sid))
        else:
            ordered = in_domain
        if not ordered:
            raise SystemExit(f"No station history covers target period {first} -> {last}")

        accepted: list[Station] = []
        decisions = []
        for st in ordered:
            success = False
            detail = ""
            for attempt in range(args.retries):
                success, detail, _ = download_one(year, st.sid, args.out_dir, args.timeout)
                if success:
                    break
            if success:
                good, msg = validate_time_completeness(
                    args.out_dir / f"{st.sid}-{year}.csv", year, st.sid)
            else:
                good, msg = False, f"download failed: {detail}"
            distance = haversine_km(stack[0], stack[1], st.lat, st.lon) if stack else None
            decisions.append({
                "sid": st.sid, "distance_to_stack_km": distance,
                "download_success": success, "raw_annual_coverage": msg,
                "selected": success and (good or mode == "OBSERVED"),
            })
            print(f"candidate year={year} sid={st.sid} mode={mode} {msg}", flush=True)
            if success and (good or mode == "OBSERVED"):
                accepted.append(st)
                if mode == "VIRTUAL_RELOCATED":
                    break
            elif mode == "OBSERVED":
                # Every in-domain station must download: no silent omissions.
                print(f"ERROR in-domain station {st.sid}: {msg}")
        report = args.out_dir / f"download_report_{year}.json"
        report.write_text(json.dumps({
            "year": year, "mode": mode, "research_bbox": bbox,
            "stack_latlon": stack, "candidates": decisions,
            "source_coverage_rule": "annual original unique observation hours >= 80% for fallback"
        }, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        if not accepted or (mode == "OBSERVED" and len(accepted) != len(in_domain)):
            raise SystemExit(f"P5 source selection failed for {year}; see {report}")
        aliases = numeric_station_ids([s.sid for s in accepted])
        manifest_rows = []
        for st in accepted:
            lat, lon = (st.lat, st.lon) if mode == "OBSERVED" else stack
            good, msg = validate_time_completeness(
                args.out_dir / f"{st.sid}-{year}.csv", year, st.sid)
            manifest_rows.append({
                "sid": st.sid, "name": st.name, "country": st.country,
                "begin": st.begin, "end": st.end, "lat": lat, "lon": lon,
                "elev": st.elev, "source_lat": st.lat, "source_lon": st.lon,
                "source_elev": st.elev, "ifstn": aliases[st.sid],
                "relocated": mode == "VIRTUAL_RELOCATED",
                "raw_annual_coverage": msg
            })
        manifest = args.out_dir / f"station_selection_{year}.json"
        manifest.write_text(json.dumps({
            "year": year, "mode": mode, "stations": manifest_rows
        }, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        listing = args.out_dir / f"surf_station_list_{year}.txt"
        with listing.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=[
                "sid", "name", "lat", "lon", "elev", "country", "begin", "end",
                "source_lat", "source_lon", "source_elev", "ifstn", "relocated"
            ], extrasaction="ignore")
            writer.writeheader()
            writer.writerows(manifest_rows)
        print(f"P5_SELECTED year={year} mode={mode} station_count={len(accepted)} "
              f"stations={','.join(st.sid for st in accepted)} manifest={manifest}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
