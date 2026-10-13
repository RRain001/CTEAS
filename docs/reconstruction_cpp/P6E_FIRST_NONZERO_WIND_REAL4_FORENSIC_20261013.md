# P6E failure: upstream native wind REAL*4 first-divergence audit (2026-10-13)

## Scope and scientific status

This is a forensic note for frozen Hefei 2022, 50×50×9 cells, 143 hours.
P6D 27/27 U/V/W source-comparator fields PASS, but **P6E is NOT yet accepted**.
Do not weaken acceptance tolerances and never feed original Fortran wind back
into the accepted C++ native candidate. All source-wind substitutions are
explicit isolated controls, not production.

## Controlled evidence

- Baseline [Actions #38362578127](https://github.com/RRain001/CTEAS/actions/runs/38362578127):
  USTAR 5,952 / EL 1,105 / WSTAR 171 outside fixed tolerances, 7,228 total.
  Source-wind input into unchanged C++ P6E produces three PASS: a one-variable
  diagnostic that implicates upstream wind numerical differences, but does not
  establish which stage caused them.
- Confirmed native-vs-original PREPDI discrepancy:
  original pinned `calmet.for` at L13890 uses
  `WDRAD = 0.0174533 * WD` with Fortran REAL*4 arithmetic, while the old
  native code used `WD * pi / 180.f`. This changes the station's U/V before
  the station flux and extrapolation calculations. Corrected in
  [commit c864c13](https://github.com/RRain001/CTEAS/commit/c864c13ff848befdb640bbf252fcae41488ac60d).
- After changing ONLY the native wind-to-radians literal, frozen numeric gate
  [Actions #38365068473](https://github.com/RRain001/CTEAS/actions/runs/38365068473):
  USTAR **1,935**, EL **236**, WSTAR **38**, **2,209** total outside the
  unchanged tolerances. Reduction 69.4%, but still FAIL. First failing
  USTAR hour 14, EL/WSTAR hour 9. P6D 27/27 remain PASS.
- Old U/V level-1 max differences 5.96e-6 / 9.54e-6 m/s, now both
  9.536743e-7 m/s. Old AVETMP integer shift flips: 6,187
  (with 187 USTAR failures), after fix 422 (with only **7**
  USTAR failures). Therefore discrete integer-shift flips cannot explain
  most remaining P6E USTAR failures.

## Exact REAL*4 source boundary trace

The write-only Fortran source instrumentation in
`scripts/p6d_trace_original_inter2_first_layer_143h.py` copies SHA-pinned
vendor CALMET and requires its original emitted MET.DAT to retain the frozen
SHA-256; it does not write into the vendor codebase.

- [Actions #38365657386](https://github.com/RRain001/CTEAS/actions/runs/38365657386)
  after radians fix: surface U/V first-hour differences now vanish to
  REAL*4 accuracy; the earlier CSV decimal comparison had negligible
  formatting-only artifacts. Original `before_slope_u/v` first level
  remains nonzero, max 1.90735e-6 / 1.43051e-6 m/s;
  `inter2_u/v` max 9.53674e-7 / 1.43051e-6 m/s.
- [Actions #38366338218](https://github.com/RRain001/CTEAS/actions/runs/38366338218)
  adds an original after-WIND1 source snapshot and compares all 143 hours to
  native `wind1_u/v_*.f32`. The **first observable nonzero wind stage is
  WIND1**, hour 1, (i=1,j=1), U Δ=1.1920929e-7,
  V Δ=2.3841858e-7 m/s. 143h WIND1 max differences
  U 1.90735e-6 and V 1.43051e-6 m/s.
  The downstream FRADJ/SLOPE/INTER2 outputs inherit this drift.

Independent synthetic first-guess WIND1 tests pass exactly zero difference,
so the real-case divergence is in **runtime preparation and/or REAL*4
arithmetic for actual station neutral SIMILT profiles, real station coordinates,
or other actual-case inputs**, not proven to be in the interpolation kernel.
Do not claim all 2,209 downstream exceedances have been causally attributed
to a single WIND1 computation until the full native-wind 143h strict comparator
and same-wind internal-stage A/B controls substantiate it.

## Next narrow acceptance experiments

1. For each hour, trace the source and native *station neutral SIMILT*
   (USTMP/VSTMP) before WIND1 interpolation and record REAL*4 bit differences;
   if identical, compare frozen real-case station coordinate and normalized
   WIND1 `RSQWT` at first differing (i,j,k).
2. Test original implicit REAL*4 intermediates and REAL*4-to-REAL*8 promotion
   at SIMILT `ELINV / DH200 / PSIM / R8` as a single bounded change,
   using strict source-stage trace and 143h comparator.
3. Re-run the original P6E fields with **native** P6D input; require
   USTAR, EL, WSTAR to have zero tolerance exceedances. Also retain
   27/27 P6D PASS, fail-closed input/output, source-case SHA, and 143h
   C++/Python coordinator byte parity.
4. Any residual should be localized by first-failing hour/cell and P6E
   `AVETMP→SURFVAR→ELUSTR→MIXHT→AVEMIX→WSTARR` stage tracing before
   modifying physics. Near-neutral EL has high condition sensitivity due to
   heat-flux denominators. No post-hoc relaxation of scientific tolerances.

**Hold:** PR #53 remains Draft and source scientific P6 G4 is FAIL
until all three native-driven P6E fields meet original fixed tolerances.
