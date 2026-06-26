import os
import re
import numpy as np
import pandas as pd


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
    """
    连续变量补值规则：
    <=2小时: 线性插值
    3-6小时: 时间插值
    >6小时: 年平均值
    """
    s = pd.to_numeric(series, errors="coerce").copy()
    # 统一缺测码
    s = s.replace(9999, np.nan)

    linear_interp = s.interpolate(method="linear", limit_area="inside")
    time_interp = s.interpolate(method="time", limit_area="inside")
    annual_mean = s.mean(skipna=True)

    filled = s.copy()
    miss_mask = filled.isna().to_numpy()
    runs = _missing_runs(miss_mask)

    for st, ed in runs:
        gap_len = ed - st + 1
        idx_slice = filled.index[st : ed + 1]
        if gap_len <= 2:
            filled.loc[idx_slice] = linear_interp.loc[idx_slice]
        elif gap_len <= 6:
            filled.loc[idx_slice] = time_interp.loc[idx_slice]
        else:
            filled.loc[idx_slice] = annual_mean

    # 边界等仍未补上的，统一用年平均兜底
    if not np.isnan(annual_mean):
        filled = filled.fillna(annual_mean)

    return filled


def fill_wind(df, dir_col, spd_col):
    """
    风向/风速缺测补值：
    先转u,v，再插值u,v，再反算风速风向。
    """
    wd = pd.to_numeric(df[dir_col], errors="coerce").replace(9999, np.nan)
    ws = pd.to_numeric(df[spd_col], errors="coerce").replace(9999, np.nan)

    # 任一缺失则u/v都设缺失
    valid = wd.notna() & ws.notna()
    rad = np.deg2rad(wd.where(valid))
    u = -ws.where(valid) * np.sin(rad)
    v = -ws.where(valid) * np.cos(rad)

    u_i = u.interpolate(method="time", limit_area="inside")
    v_i = v.interpolate(method="time", limit_area="inside")

    # 插值后剩余缺失，用全年均值兜底
    u_mean = u.mean(skipna=True)
    v_mean = v.mean(skipna=True)
    if not np.isnan(u_mean):
        u_i = u_i.fillna(u_mean)
    if not np.isnan(v_mean):
        v_i = v_i.fillna(v_mean)

    ws_new = np.sqrt(u_i**2 + v_i**2)
    wd_new = (np.degrees(np.arctan2(-u_i, -v_i)) + 360) % 360

    # 非法值兜底
    ws_new = ws_new.fillna(9999)
    wd_new = wd_new.fillna(9999)

    # 平静风向设0
    wd_new = np.where(ws_new <= 1e-6, 0, wd_new)
    # 非静风时，0度等价写为360（符合常见气象编码习惯）
    wd_new = np.where((ws_new > 1e-6) & (np.isclose(wd_new, 0)), 360, wd_new)

    df[spd_col] = ws_new.round(3)
    df[dir_col] = np.round(wd_new, 0).astype(int)
    return df


def process_one_station(file_path):
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

    # 连续变量补值规则
    for col in [temp_col, rh_col, pressure_col]:
        if col and col in df.columns:
            df[col] = fill_continuous_by_gap_rule(df[col])

    # 风场：u/v插值后反算
    if wdir_col and wspd_col and wdir_col in df.columns and wspd_col in df.columns:
        df = fill_wind(df, wdir_col, wspd_col)

    # 指定列缺失值固定填充
    if precip_col and precip_col in df.columns:
        df[precip_col] = pd.to_numeric(df[precip_col], errors="coerce").replace(-9999, np.nan).fillna(9999)
    if pressure_col and pressure_col in df.columns:
        df[pressure_col] = pd.to_numeric(df[pressure_col], errors="coerce").replace(-9999, np.nan).fillna(9999)
    if ccover_col and ccover_col in df.columns:
        df[ccover_col] = pd.to_numeric(df[ccover_col], errors="coerce").replace(-9999, np.nan).fillna(9999)
    if cheight_col and cheight_col in df.columns:
        df[cheight_col] = (
            pd.to_numeric(df[cheight_col], errors="coerce")
            .replace(-9999, np.nan)
            .fillna(722)
            .astype(int)
        )

    # 按原始第三行列顺序输出
    out_cols = [c for c in columns if c in df.columns]
    df_out = df[out_cols].copy()

    # 整数列格式化
    for int_col in ["Year", "Month", "Day", "Hour"]:
        if int_col in df_out.columns:
            df_out[int_col] = df_out[int_col].astype(int)

    return header_lines, df_out


def save_station_csv(file_path, header_lines, df_out):
    base, ext = os.path.splitext(file_path)
    output_path = f"{base}_filled{ext}"
    with open(output_path, "w", encoding="utf-8", newline="") as f:
        for line in header_lines:
            f.write(line + "\n")
        df_out.to_csv(f, index=False, header=False)
    return output_path


if __name__ == "__main__":
    workdir = "."
    pattern = re.compile(r"^\d{5}0-99999-\d{4}\.csv$")
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
        headers, df_result = process_one_station(input_file)
        _out = save_station_csv(input_file, headers, df_result)
