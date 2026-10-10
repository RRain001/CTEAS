# P4 READ62 v5.661 L110225: C++20 checkpoint

Status 2026-10-10: PASS_SOFTWARE_SCOPED_LOCAL / BLOCKED_FORTRAN_REFERENCE / NOT_A_READ62_REPLACEMENT.

Base commit: 60b7b1d93866fff772926b6ac32dc0af7235552b. Original READ62.FOR blob 2de05fbeb32cc3aca731a9e89b0a0889c6cafab9, read62.inp blob 5363fad287c68d32cf7f9691ca7ff173f59b6261. The original code was not modified.

Implemented: standalone C++20 JDAT=2 four-header FSL decoder, station UTC time/coordinates/elevation, fixed-width profile, MXLEV=200, ms/kt conversion, explicit per-field missing masks, original monotonic-pressure discard rule, optional missing-field drops, surface/PSTOP/height/time diagnostics. Separate CMake module and synthetic contract tests; no shared API or parent CMake changes.

Run locally: cmake -S native_cpp/modules/p4_read62 -B /tmp/cteas-p4-build && cmake --build /tmp/cteas-p4-build && ctest --test-dir /tmp/cteas-p4-build --output-on-failure

Not implemented: TD-6201 JDAT=1; substitute ISUB; repair LXTOP/LXSFC; full QA; time-window control; formatted UP.DAT IFMT=1/2 writer; READ62.LST. Current bundled controls use year 2000; verified 2022 actual station files, original run and original output are unavailable. The tests do not establish numerical/byte parity.

Exit gate: freeze actual Hefei 2022 source inputs/controls, compile/run READ62 reference in isolated workdir, implement selected remaining controls and writer, compare station/profile timestamps, valid/missing denominators, P/Z/T/WD/WS with predetermined tolerances and independent audit.
