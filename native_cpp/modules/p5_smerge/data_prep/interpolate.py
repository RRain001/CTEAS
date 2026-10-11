import json
import os
import re
import numpy as np
import pandas as pd
from .wind_policy_v2 import clean_then_replace_calms, WindPolicyError


def read_station_csv(file_path):
    """读取3行头的站点CSV"""
    with open(file_path, "r", encoding="utf-8") as f:
        header_lines = [f.readline().rstrip("\n") for _ in range(3)]

    columns = [x.strip() for x in header_lines[2].split(",")]
    df = pd.read_csv(file_path, skiprows=3, header=None, names=columns)
    return header_lines, columns, df


def build_hourly_index(df):
    """构造时间索引并补齐全年逐小时"""
    hour_val = (df["Hour"].astype(float) / 100).astype(int)
    dt = pd.to_datetime(
        {
            "year": df["Year"].astype(int),
            "month": df["Month"].astype(int),
            "day": df["Day"].astype(int),
            "hour": hour_val,
        }
    )

    out = df.copy()
    out["Datetime"] = dt
    out = out.set_index("Datetime").sort_index()

    year = int(out["Year"].mode().iloc[0])
    full_index = pd.date_range(f"{year}-01-01 00:00:00", f"{year}-12-31 23:00:00", freq="h")
    out = out.reindex(full_index)
    return out


def _missing_runs(mask):
    """返回缺失段列表[(start, end), ...], end为闭区间"""
    runs = []
    n = len(mask)
    i = 0
    while i < n:
        if not mask[i]:
            i += 1
            continue
        start = i
        while i + 1 < n and mask[i + 1]:
            i += 1
        runs.append((start, i))
        i += 1
    return runs


def fill_continuous_by_gap_rule(series):
    """Previous valid hourly value, no maximum gap; no look-ahead or annual means.

    A missing sequence at the beginning remains missing (9999 on output).
    The precipitation and cloud treatment is intentionally unchanged.
    """
    s = pd.to_numeric(series, errors="coerce").replace([9999, -9999], np.nan)
    return s.ffill()


CALM_LIMIT_M_S = 0.5


class WindPreparationError(ValueError):
    """Atmos 同型错误:稳定错误码 + 时段范围,不做静默替换。"""

    def __init__(self, code, ranges):
        super().__init__(code)
        self.code = code
        self.ranges = ranges

    @property
    def station_less_ranges(self):
        return [
            {key: value for key, value in item.items() if key != "station_id"}
            for item in self.ranges
        ]


def fill_wind_forward_calm(df, dir_col, spd_col, strict=False):
    """Scheme C: preserve measured/forward-held wind for GENERIC and CALMET.

    Legacy below-0.5 low-wind tagging is audit-only. Numerical minima,
    extrapolation and true calm handling are delegated to original CALMET.
    An exact 0 m/s wind receives canonical WD=0 in GENERIC because direction
    is physically undefined; raw wind direction remains in source and audit.
    """
    index = df.index
    direction = pd.to_numeric(df[dir_col], errors="coerce").replace(9999, np.nan)
    speed = pd.to_numeric(df[spd_col], errors="coerce").replace(9999, np.nan)

    dir_src = pd.Series(pd.NaT, index=index).astype("datetime64[ns]")
    dir_src[direction.notna()] = index[direction.notna()]
    spd_src = pd.Series(pd.NaT, index=index).astype("datetime64[ns]")
    spd_src[speed.notna()] = index[speed.notna()]

    # A calm observation's WD=0 is only a semantic marker, NOT a valid
    # remembered wind bearing. Recalling that 0 as the direction for a later
    # non-calm observation with missing WD would fabricate a north wind.
    direction_seed = direction.mask(speed.eq(0.0))
    dir_src = dir_src.mask(speed.eq(0.0))
    direction_filled = direction_seed.ffill()
    speed_filled = speed.ffill()
    dir_src = dir_src.ffill()
    spd_src = spd_src.ffill()

    exact_zero = speed_filled.eq(0.0).fillna(False)
    observation_case = pd.Series("BOTH_MISSING", index=index, dtype="object")
    observation_case.loc[direction.notna() & speed.notna()] = "BOTH_PRESENT"
    observation_case.loc[direction.isna() & speed.notna()] = "SPEED_ONLY"
    observation_case.loc[direction.notna() & speed.isna()] = "DIRECTION_ONLY"
    observation_case.loc[exact_zero & speed.notna()] = "CALM_OBSERVED"
    observation_case.loc[exact_zero & speed.isna()] = "CALM_PREVIOUS_SPEED"
    direction_generic = direction_filled.mask(exact_zero, 0.0)
    speed_generic = speed_filled.copy()

    leading = []
    for field, values in (("wind_direction", direction_generic),
                          ("wind_speed", speed_generic)):
        missing = values.isna().to_numpy()
        count = 0
        for flag in missing:
            if not flag:
                break
            count += 1
        if count:
            leading.append({
                "field": field,
                "start_utc": index[0],
                "end_utc_inclusive": index[count - 1],
                "hours": count,
            })
        # After leading CALM hours, an unseeded positive-speed WD may be
        # missing in the interior: calm WD=0 is not a valid historical
        # direction. Retain missing; SMERGE will invalidate the incomplete
        # wind pair. Do not manufacture a northerly wind.
    unseeded_noncalm = direction_generic.isna() & speed_generic.gt(0)
    if unseeded_noncalm.any():
        first_bad = index[int(np.flatnonzero(unseeded_noncalm.to_numpy())[0])]
        leading.append({
            "field": "wind_direction_unseeded_noncalm",
            "start_utc": first_bad,
            "end_utc_inclusive": index[-1],
            "hours": int(unseeded_noncalm.sum()),
        })

    # 0.5 m/s is ONLY a historical P5 diagnostic category, not a clamp.
    low_wind = speed_filled.lt(CALM_LIMIT_M_S).fillna(False)
    segments = _missing_runs(low_wind.to_numpy())
    boundary_segments = []
    for left, right in segments:
        if left == 0 or right == len(index) - 1:
            boundary_segments.append({
                "start_utc": index[left],
                "end_utc_inclusive": index[right],
                "hours": right - left + 1,
                "missing_before_noncalm": left == 0,
                "missing_after_noncalm": right == len(index) - 1,
            })

    if strict and leading:
        raise WindPreparationError("LEADING_MISSING_NO_PREDECESSOR", leading)

    method = pd.Series("OBSERVED_OR_PREVIOUS_HOUR", index=index, dtype="object")
    method.loc[exact_zero] = "ZERO_SPEED_DIRECTION_CANONICAL_0_FOR_GENERIC"
    dir_age_h = (pd.Series(index, index=index) - dir_src).dt.total_seconds() / 3600
    spd_age_h = (pd.Series(index, index=index) - spd_src).dt.total_seconds() / 3600
    pair_valid = direction_generic.notna() & speed_generic.notna()
    audit = pd.DataFrame({
        "wind_direction_raw_deg": direction,
        "wind_speed_raw_m_s": speed,
        "observation_case": observation_case,
        "unseeded_noncalm_direction": unseeded_noncalm,
        "generic_wind_pair_valid": pair_valid,
        "wind_direction_filled_deg": direction_filled,
        "wind_direction_was_filled": direction_seed.isna() & direction_filled.notna(),
        "wind_direction_fill_source_utc": dir_src,
        "wind_direction_source_age_hours": dir_age_h,
        "wind_speed_filled_m_s": speed_filled,
        "wind_speed_was_filled": speed.isna() & speed_filled.notna(),
        "wind_speed_fill_source_utc": spd_src,
        "wind_speed_source_age_hours": spd_age_h,
        "is_calm": low_wind,
        "is_exact_zero_speed": exact_zero,
        "wind_direction_statistics_deg": direction_filled,
        "wind_speed_statistics_m_s": speed_filled,
        "wind_direction_generic_deg": direction_generic,
        "wind_speed_generic_m_s": speed_generic,
        "wind_speed_calculation_m_s": speed_generic,
        "direction_statistics_method": method,
    }, index=index)

    summary = {
        "wind_policy": "P5_C_SOURCE_WIND_V1",
        "calm_limit_m_s": CALM_LIMIT_M_S,
        "calm_limit_usage": "LOW_SPEED_DIAGNOSTIC_ONLY",
        "hour_count": int(len(index)),
        "calm_hours": int(low_wind.sum()),
        "exact_zero_hours": int(exact_zero.sum()),
        "direction_forward_filled_hours": int((direction_seed.isna() & direction_filled.notna()).sum()),
        "speed_forward_filled_hours": int((speed.isna() & speed_filled.notna()).sum()),
        "calm_segments": len(segments),
        "leading_missing": leading,
        "boundary_calm_segments": boundary_segments,
        "observed_speed_only_hours": int((observation_case == "SPEED_ONLY").sum()),
        "observed_direction_only_hours": int((observation_case == "DIRECTION_ONLY").sum()),
        "observed_both_missing_hours": int((observation_case == "BOTH_MISSING").sum()),
        "unseeded_noncalm_direction_hours": int(unseeded_noncalm.sum()),
        "generic_valid_wind_pair_hours": int(pair_valid.sum()),
        "generic_invalid_wind_pair_hours": int((~pair_valid).sum()),
        "max_wind_direction_previous_value_hours": float(dir_age_h.max()) if dir_age_h.notna().any() else None,
        "max_wind_speed_previous_value_hours": float(spd_age_h.max()) if spd_age_h.notna().any() else None,
        "statistics_speed_field": "wind_speed_filled_m_s",
        "calculation_speed_field": "wind_speed_generic_m_s (NO P5 MINIMUM)",
        "main_table_speed_field": "wind_speed_generic_m_s (zero and low wind retained)",
        "main_table_direction_field": "wind_direction_generic_deg (zero only canonicalized)",
        "calmet_calm_handling": "DEFERRED_TO_ORIGINAL_CALMET_SOLVER",
    }

    out = df.copy()
    out[spd_col] = speed_generic.round(3).fillna(9999)
    out[dir_col] = direction_generic.round(0).fillna(9999).astype(int)
    return out, audit, summary


def write_wind_audit(file_path, audit, summary):
    """在主表旁写审计文件:逐时填充来源/静风/统计风向 + 网关汇总。"""
    base, ext = os.path.splitext(file_path)
    audit_path = f"{base}_wind_audit{ext}"
    out = audit.copy()
    for column in ("previous_valid_noncalm_utc", "calm_left_valid_utc", "calm_right_valid_utc"):
        if column in out.columns:
            out[column] = out[column].dt.strftime("%Y-%m-%dT%H:%M:%SZ")
    out.insert(0, "at_utc", out.index.strftime("%Y-%m-%dT%H:%M:%SZ"))
    out.to_csv(audit_path, index=False)

    payload = dict(summary)
    payload["station_file"] = os.path.basename(file_path)
    summary_path = f"{base}_wind_audit.json"
    with open(summary_path, "w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, ensure_ascii=False)
        handle.write("\n")
    return audit_path, summary_path


def process_one_station(file_path, strict=False):
    header_lines, columns, df_raw = read_station_csv(file_path)
    df = build_hourly_index(df_raw)

    # 重建基础时间列
    df["Year"] = df.index.year
    df["Month"] = df.index.month
    df["Day"] = df.index.day
    df["Hour"] = df.index.hour * 100

    # 列映射（兼容你当前文件和可能的别名）
    precip_col = "mm" if "mm" in df.columns else ("Precip" if "Precip" in df.columns else None)
    pressure_col = "mb" if "mb" in df.columns else ("Pressure" if "Pressure" in df.columns else None)
    ccover_col = "tenths" if "tenths" in df.columns else ("Ccover" if "Ccover" in df.columns else None)
    cheight_col = "hundreds_of_feet" if "hundreds_of_feet" in df.columns else ("Cheight" if "Cheight" in df.columns else None)
    temp_col = "DegC" if "DegC" in df.columns else ("Temp" if "Temp" in df.columns else None)
    rh_col = "%" if "%" in df.columns else ("RH" if "RH" in df.columns else None)
    wdir_col = "deg" if "deg" in df.columns else ("Wdir10m" if "Wdir10m" in df.columns else None)
    wspd_col = "ms-1" if "ms-1" in df.columns else ("Wspeed10m" if "Wspeed10m" in df.columns else None)

    # All nonwind variables: previous valid value, unlimited carry.
    # Never treat source-missing precipitation/cloud as a real zero or
    # invent a default cloud-ceiling height. Genuine no-cloud 999 is kept.
    nonwind = [temp_col, rh_col, pressure_col, precip_col, ccover_col, cheight_col]
    for col in nonwind:
        if col and col in df.columns:
            df[col] = fill_continuous_by_gap_rule(df[col])

    # The original-hour presence must come from ISD-Lite, not the UNION
    # of two source datasets. An hour with raw Lite -9999 still counts.
    from pathlib import Path
    source = Path(file_path)
    source_audit = source.with_name(source.stem + "_isd_lite_source_audit.csv")
    if not source_audit.is_file():
        raise WindPolicyError(f"missing mandatory raw ISD-Lite hourly presence: {source_audit}")
    provenance = pd.read_csv(source_audit)
    if not {"at_utc", "lite_report_present"}.issubset(provenance.columns):
        raise WindPolicyError("ISD-Lite provenance required columns missing")
    source_times = pd.to_datetime(provenance["at_utc"], utc=True).dt.tz_localize(None)
    raw_presence = pd.Series(provenance["lite_report_present"].astype(bool).to_numpy(),
                             index=pd.DatetimeIndex(source_times))
    if not raw_presence.index.is_unique:
        raise WindPolicyError("duplicated original source UTC hour")
    if (raw_presence.index.difference(df.index)).size:
        raise WindPolicyError("ISD-Lite provenance contains hour outside reconstructed year")

    if not (wdir_col and wspd_col):
        raise WindPolicyError("missing required GENERIC wind columns")
    cleaned, modeled, audit, summary = clean_then_replace_calms(
        df, wdir_col, wspd_col, raw_presence, strict=strict)
    df = modeled

    # The copied P5 serializer writes stage-1 + stage-2 files; final
    # downstream GENERIC still uses *_filled.csv for compatibility.
    cleaned_for_export = cleaned.copy()
    for col in [wdir_col, wspd_col]:
        cleaned_for_export[col] = cleaned_for_export[col].fillna(9999)
    for col in [wdir_col, wspd_col]:
        df[col] = df[col].fillna(9999)

    # 按原始第三行列顺序输出
    out_cols = [c for c in columns if c in df.columns]
    # Preserve GENERIC's 9999 missing sentinel even for leading nonwind gaps.
    df_out = df[out_cols].copy().fillna(9999)
    df_cleaned = cleaned_for_export[out_cols].copy().fillna(9999)

    # 整数列格式化
    for int_col in ["Year", "Month", "Day", "Hour"]:
        if int_col in df_out.columns:
            df_out[int_col] = df_out[int_col].astype(int)
            df_cleaned[int_col] = df_cleaned[int_col].astype(int)

    df_out.attrs["stage1_cleaned_df"] = df_cleaned
    df_out.attrs["p5_policy_summary"] = summary
    return header_lines, df_out, audit, summary


def save_station_csv(file_path, header_lines, df_out):
    """Keep auditable stage-1 + stage-2 files and old downstream final path."""
    base, ext = os.path.splitext(file_path)
    cleaned = df_out.attrs.get("stage1_cleaned_df")
    if cleaned is None:
        raise WindPolicyError("stage-1 cleaned wind file is mandatory")
    paths = {
        "cleaned": f"{base}_cleaned{ext}",
        "calm_interpolated": f"{base}_calm_interpolated{ext}",
        "final": f"{base}_filled{ext}",
    }
    for role, data in (("cleaned", cleaned),
                       ("calm_interpolated", df_out),
                       ("final", df_out)):
        with open(paths[role], "w", encoding="utf-8", newline="") as out:
            for header in header_lines:
                out.write(header + "\n")
            data.to_csv(out, index=False, header=False)
        # GENERIC's fixed 3-line header cannot contain arbitrary stage labels.
        # Explicit UTF-8 JSON sidecar declares the exact scientific stage.
        status = ("数据清洗和缺测处理完毕" if role == "cleaned"
                  else "静风插值处理完毕")
        summary = df_out.attrs.get("p5_policy_summary", {})
        sidecar = paths[role].replace(".csv", ".stage.json")
        with open(sidecar, "w", encoding="utf-8") as meta:
            json.dump({
                "status": status,
                "stage": role,
                "file_name": os.path.basename(paths[role]),
                "wind_policy": summary.get("policy"),
                "original_calm_hours": summary.get("original_calm_hours"),
                "raw_absent_hours": summary.get("raw_absent_hours"),
                "year": summary.get("year"),
                "calendar_hours": summary.get("expected_calendar_hours"),
            }, meta, ensure_ascii=False, indent=2)
            meta.write("\n")
    return paths["final"]


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="P5 v2: annual whole-vector ffill, then original-calm replacement")
    parser.add_argument("--strict", action="store_true",
                        help="缺少前置非静风观测时要求失败")
    args = parser.parse_args()

    workdir = "."
    pattern = re.compile(r"^[0-9A-Za-z]{6}-[0-9A-Za-z]{5}-\d{4}\.csv$")
    candidates = []
    for x in os.listdir(workdir):
        _, ext = os.path.splitext(x)
        # 仅识别CSV后缀文件
        if ext.lower() == ".csv" and pattern.match(x):
            candidates.append(x)

    if not candidates:
        print("未找到符合命名规则的站点CSV(如 583380-99999-2023.csv)")
        raise SystemExit(1)

    candidates = sorted(candidates)

    for input_file in candidates:
        try:
            headers, df_result, audit, summary = process_one_station(input_file, strict=args.strict)
        except WindPreparationError as error:
            print(f"[{input_file}] {error.code}: 缺乏前置观测，未生成派生结果")
            for item in error.station_less_ranges:
                print(f"    {item}")
            raise SystemExit(1)
        _out = save_station_csv(input_file, headers, df_result)
        if audit is not None:
            audit_path, summary_path = write_wind_audit(_out, audit, summary)
            print(f"[{input_file}] 审计: {os.path.basename(audit_path)} / {os.path.basename(summary_path)}")
