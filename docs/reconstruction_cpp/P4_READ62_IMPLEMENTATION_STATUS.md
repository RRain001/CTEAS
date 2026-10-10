# P4 READ62 v5.661 L110225: scoped C++20 reproduction

Status (2026-10-10): **PASS_JDAT2_IFMT1_FULL_YEAR_NUMERIC_AND_BYTE_PARITY** / **PASS_CPP_CALMET_FINAL_PUBLICATION** / **UNSUPPORTED_LEGACY_ALTERNATE_OPTIONS**.

## Authority and source protection

- Frozen original `engine/vendor/src/READ62_v5.661_L110225/READ62.FOR` and associated legacy INP/source files are unchanged; the Ubuntu compiler runs the original source with temporary case-normalized include aliases and 72-column fixed-form syntax.
- Original Python `engine/preprocess/read62_upperair_data/` remains unchanged. `download_igra.py`, `parse_igra_to_csv.py`, `read.py`, source README and strict requirements were originally copied into `native_cpp/modules/p4_read62/data_prep/`. `merge_final_upperair.py` is now also copied **byte-identically** for final-stage publishing. Only the scoped copies of data download/parse/FSL are changed.
- Branch: `codex/cteas-p4-read62-reference`, Draft PR #6, no merge to `main`.

## Supported and implemented

- Annual station selection: valid distinct **original cleaned** 00/12 UTC sounding slots (not copied missing intervals) / 730 or 732; **strictly greater than 80%**. All qualifying in-domain stations remain at true coordinates. If none qualifies, one nearest passing external station is relocated horizontally to chimney coordinates, while source coordinates and original elevation remain auditable; repeat independently every year.
- Preserve original missing-profile cloning and first-level wind/temperature repairs. For targeted pressure-level insertion, use shortest circular wind direction arc, 180-degree ties clockwise.
- FSL generated from copied upstream Python. Legacy fixed-width latitude F7.2 / longitude F6.2 remain intact; optional `P4EXACT_LAT=...` and `P4EXACT_LON=...` suffixes with 3–6 decimals are accepted by new C++ while ignored by original Fortran.
- Independent C++20 `cteas_p4_read62` reads FSL and writes the READ62 `JDAT=2, IFMT=1, PSTOP=500 hPa, LHT/LTEMP/LWD/LWS=F, ISUB=0, LXTOP/LXSFC=F` UP.DAT output. Scoped Python runner `run_cpp_pipeline.py` uses it per station/year, then the copied legacy merger builds CALMET case files and exact station metadata.

## Independent production-relevant evidence

Actual public NOAA IGRA `CHM00058424` ANQING 2022, 5×5 km Hefei model domain, 100m grid; source lat/lon 30.6167/116.9667, effective lat/lon 31.939774444/117.141610855, station source elevation 62 m, 728/730=99.726% valid original 00/12 UTC soundings.

- 2022-01-01 through 2022-01-06: **12/12 profiles, 729/729 levels, 3,645/3,645 numerical fields identical** to original READ62.
- **Full 2022:** **730/730 profiles, 42,577/42,577 levels, 212,885/212,885 P/Z/T/WD/WS numerical fields identical** to original READ62. The single original top-level missing-data annotation was reproduced.
- Both six-day and full-year **UP.DAT byte-for-byte equality** is enforced by dedicated GitHub Actions `cmp`.
- Coordinate suffix precision **3/4/5/6 decimals**: four actual Fortran/C++ runs; original Fortran UP.DAT byte-identical to baseline; new C++ parses the appended exact coordinate with corresponding precision and produces identical numerical UP.DAT.
- Case pipeline `run_cpp_pipeline.py` publishes `project/current/met/upperair/final/up_58424.dat`, `stations.csv`, and an execution manifest. Final legacy merger clips to CALMET's 99-level limit and adds one boundary record; this is a separate transformation from the raw UP.DAT Fortran/C++ parity input.

CI: `https://github.com/RRain001/CTEAS/actions/workflows/p4-read62-fortran-parity.yml`. See PR #6 for SHA-locked run and artifact links.

## Explicit limitations

- Does **not** claim equivalence for TD-6201 `JDAT=1`, `IFMT=2` comma records, substitute profiles `ISUB>0`, advanced original READ62 repair switches `LXTOP/LXSFC`, non-default missing-layer dropping, alternate climate/operating settings, or other IGRA stations. The frozen eligible case only needs `JDAT=2, IFMT=1` defaults.
- The original Fortran **still reads 2 decimals** from legacy fields. The optional suffix carries up to 6 decimal precision to C++; this does not change original Fortran computations or historical compatibility.
- Moving off-site observations to chimney is a **virtual proxy** and cannot be described as direct site measurements.
- Multi-year annual reselection is implemented for inputs. Before CALMET production with yearly changes in station membership/location, review station identity and time-dependent metadata compatibility rather than assuming static station continuity.
- No Fortran `READ62.LST` reproduction in C++ (Fortran log exists as independent audit); final CALMET P6 physical wind equivalence is a separate module.
