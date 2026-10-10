# P5 SMERGE v5.7.0 L121203: C++20 checkpoint

Status 2026-10-10: PASS_SOFTWARE_SCOPED_LOCAL / BLOCKED_FORTRAN_REFERENCE / NOT_A_SMERGE_REPLACEMENT.

Base commit 60b7b1d93866fff772926b6ac32dc0af7235552b. Original SMERGE.FOR blob a4315036dae437977d9c90ab5abbb8b8085018d5 and smerge.inp blob 01638b034174a27383ed9ec91486253a7365aabe are untouched.

Implemented: scoped JDAT=8 GENERIC v2.0 12-column station decoder, IFSTN ID and units checks, explicit missing value masks, degC-to-K, wind pair logic, precipitation 0/1/2, exact UTC hours and dates, duplicate/order checks, deterministic multi-station hourly assembly with missing-hour counts. Reject unsupported ASTZ/ABTZ offsets. Separate CMake and tests; no shared API edits.

Run locally: cmake -S native_cpp/modules/p5_smerge -B /tmp/cteas-p5-build && cmake --build /tmp/cteas-p5-build && ctest --test-dir /tmp/cteas-p5-build --output-on-failure

Not implemented: other JDAT formats, original quality rules, timezone conversion, LPCALC, LPREV, original formatted or binary SURF.DAT (IOFORM/IOPACK) writer, original listing. No source-data imputation is performed. Current bundled controls are year 2000 and 2022 station CSVs and immutable Fortran reference output are unavailable. No scientific/byte parity claimed.

Exit gate: freeze Hefei 2022 per-station source files and exact controls, capture original SMERGE output, implement selected writer/control logic, compare per-hour/station fields, missing denominators and independent numerical audit.
