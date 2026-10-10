# P4 meteorological data preparation — annual >80% observational coverage gate

Source authority: RRain001/CTEAS main at 66e30ffb1ff4638a97af8a060748ce30b7578350 (2026-10-10).
This folder is a **copy** of upstream Python input preparation, not a new downloader and not a Fortran replacement.

| Destination | Original under engine/preprocess/read62_upperair_data | Git blob SHA |
|---|---|---|
| download_igra.py | download_igra.py | 4a1f87e740267e311905fe3270a76c445eecd49e |
| parse_igra_to_csv.py | parse_igra_to_csv.py | fae921de6f40095c079a367daf96b20e48004d71 |
| read.py | read.py | 3918d8d18cd4e30915cc27c87260f98ade249d7e |
| upstream_README.md | README.md | f54288c41d12ac922c2ed5cb71d5a85d63f507f9 |
| READ62_strict_requirements.md | READ62_strict_requirements.md | 5bef64f01b1577f15b1bc07b738512e868908e02 |

The Python input scripts were originally copied as byte-identical Git blobs. Only this scoped P4 copy is now modified; engine/preprocess is unchanged. They continue to import engine.core.paths, and use existing project/current and engine/resources/igra/igra2-station-list.txt. The ignored project/current case/data and external IGRA records are not committed here.

Scoped orchestration (no original Fortran):
- Inspect command plan safely from repo root: python3 -m native_cpp.modules.p4_read62.data_prep.prepare_inputs --year 2022 --pstop 500
- Opt-in real download/processing: append --execute
- To reuse existing downloads: --skip-download --execute

The flow is IGRA access/data-por -> original parse_igra_to_csv.py -> original read.py -> year-specific igra2fsl/*.txt. It does NOT call original run_read62.py, merge_final_upperair.py or UP.DAT writer. Original run functions remain at their upstream paths for later isolated reference runs.

Important scientific audit blockers carried from the upstream snapshot:
1. Each year independently: ALL in-domain stations passing strictly >80% valid cleaned ORIGINAL 00/12 UTC profiles are selected with real coordinates. If none passes, pick exactly ONE nearest passing station outside, relocate the model-effective lat/lon to chimney center_location, and preserve original source coordinates plus audit. Do not label a relocated sounding as site-measured observational truth.
2. parse_igra_to_csv.py and read.py may repair/extrapolate or synthesize levels; freeze processing options and label derived versus observed records. Fortran reference runs must use exactly matched input files, with hashes.
3. New data access needs network, an actual project/current case, IGRA station list, and selected year; GitHub-hosted unit CI intentionally does not download data.

P4 scientific gate stays BLOCKED_FORTRAN_REFERENCE until actual frozen 2022 inputs and original outputs are captured and compared.

## Approved P4 selection/coverage correction
- Denominator = 730 for a 365-day year, 732 for a 366-day year. Numerator = original distinct 00/12 UTC profiles passing the EXISTING per-profile cleaning/lowest-level repair; no synthetic cloned sounding is counted. Eligibility is strictly >80%, not >=80%.
- Selection is **independent for every year**, default --year omitted reads research_time and processes each year. No first-year station locking.
- Selected metadata CSV stores original source coordinates and effective model coordinates. Out-of-domain proxy uses chimney coordinates from grid/center_location, source elevation retained as requested.
- Target pressure-level wind interpolation uses the shortest circular arc, with an exact 180-degree tie clockwise (P5 convention). Existing 00/12-time copying and near-ground repair are unchanged.
- Legacy Fortran READ62.FOR FSL parser uses F7.2,a1,f6.2,a1: at longitude ~117 deg, 3–6 decimal digits do not fit. FSL remains 2 decimal places for compatibility. Retain full precision in separate CSV/audit metadata; do not change fixed-width FSL without changing and validating READ62.
- P4 is still a preparation-only stage; Fortran execution/UP.DAT parity have NOT been proven by this patch.
