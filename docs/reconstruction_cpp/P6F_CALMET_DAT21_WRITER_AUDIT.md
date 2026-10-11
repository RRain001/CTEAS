# P6F CALMET.DAT 2.1 writer — byte-identity contract

Date: 2026-10-11. Unchanged reference is CALMET 6.5.0 L150223 (original vendor Git blob
`437f43bb7f0e51a1919e56664663e1a046986ed0`). New module: `native_cpp/modules/p6_calmet/p6f/`
(branch `codex/p6f-calmet-dat21-writer-20261011`). Roadmap row: P6F `CALMET.DAT` writer in
`P6_A_TO_G_SEQUENTIAL_EXECUTION_LEDGER.md`.

## Implemented

A standalone C++20 writer that turns **explicit field arrays** into a legal CALMET.DAT 2.1 file —
the record layout the unmodified CALPUFF reads.

1. **Library** `cteas::p6f` (`include/cteas/p6f/calmet_dat21_writer.hpp`, header-declared /
   compiled in `src/`). `Calmet21File` carries the declaration record, the comment section, the
   run control block (`IBYRN..IESECN`, `AXTZ`, `IRLG`, `IRTYPE`, `NX/NY/NZ`, `DGRID`, origins,
   `IWFCOD`, `NSSTA/NUSTA/NPSTA/NOWSTA`, `NLU`, `IWAT1/IWAT2`, `LCALGRD`) plus the OUTHD
   projection block (`PMAP/DATUM/DATEN/FEAST/FNORTH/UTMHEM/IUTMZN/RNLAT0/RELON0/XLAT1/XLAT2`),
   and the explicit arrays: `ZFACE`, `XSSTA/YSSTA`, `XUSTA/YUSTA`, `XPSTA/YPSTA`,
   `Z0`, `ILANDU`, `ELEV`, `XLAI`, `NEARS`, and per interval the per-level `U-LEV`/`V-LEV`/
   `WFACE` sequence, the `T-LEV` block and `IPGT/USTAR/ZI/EL/WSTAR/[RMM]/TEMPK/RHO/QSW/IRH/[IPCODE]`.
   `serialize_calmet_dat21` / `validate_calmet_dat21` / `parse_calmet_dat21` / `read_calmet_dat21` /
   `write_calmet_dat21` / `field_inventory` are the public surface.

2. **Layout facts** are taken from the unchanged original sources, not from a decoder:
   `OUTHD` L12448–L12537 (declaration, comment section, controls word order, static record order and
   widths, `ncom=ncommout+2`), `OUTHR` L13067–L13237 (hourly order: `U/V/[W]` inside the level loop,
   then `[T-LEV]`, then `IPGT USTAR ZI EL WSTAR [RMM] TEMPK RHO QSW IRH [IPCODE]`,
   `clabel(6:8)` I3 level digits), `WRTR1D` L36789–L36816, `WRTR2D` L36829–L36881,
   `WRTI2D` L36734–L36788 (24-byte record header: `char*8` label + begin/end `YYYYJJJHH` + seconds,
   then `NX*NY` words), `map.met` L5–L7 (`character*8 datum,pmap`, `character*12 daten`,
   `character*4 utmhem`).

3. **Explicit semantics**, all enforced fail-closed:
   - *time*: packed `YYYYJJJHH` + explicit seconds; every interval exactly 3600 s; the original writes
     the end clock as the same packed hour with seconds advanced by 3600; intervals are contiguous;
     `AXTZ` is an 8-character zone label; the declared run window (`IBYRN..IESECN`) must equal the
     first interval begin and the last interval end as instants.
   - *units*: every field carries its SI unit and its OUTHR/OUTHD mask condition
     (`hourly_field_specs`, `static_field_specs`); `check` prints them next to the denominators.
   - *masks*: the field set is exactly a function of `IRTYPE`/`LCALGRD`/`NPSTA`. `RMM`/`IPCODE` with
     `NPSTA=0` are rejected (`cannot appear when NPSTA=0`), internal CALMET state with no file
     representation (`ZICONV`, `DPTT`, `QH`, `USTAROLD`) is rejected by name, a missing field of a
     declared interval is rejected, and `NEARS` supplied with `NSSTA<1` is rejected instead of being
     dropped.
   - *extents/kind*: grid, station, static and per-field value counts must match the declared
     dimensions; `IPGT`/`IRH` must be stored as int32 and every other field as real32; real values
     must be finite; `ZFACE` must be strictly increasing with `NZ+1` faces; `IWAT1<=IWAT2`.
   - *missing*: the original sentinel `XMISS=9999.` (real) / `9999` (integer) is a legal value and is
     preserved bit-exactly; `field_inventory` counts it and excludes it from the number of defined
     cells.

4. **CLI** `cteas_p6f_calmet_dat21_writer`: `roundtrip --file [--out] [--expect-sha256] [--report]`
   (parse → serialise → byte compare, with the first differing record reported),
   `check --file [--report]` (layout validation + per-field inventory with units and masks),
   `emit-fixture --out …` (deterministic, explicitly labelled synthetic layout fixture that exercises
   the writer from explicit arrays).

## Byte-identity evidence against the frozen real case

Frozen truth: `project/cases/hefei5km_2022_reference_case_20261011/run/met/met.dat`,
64,762,556 B, sha256 `b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30`,
7,562 Fortran records, 1,114 comment records, 143 intervals × 45 fields = 6,435 field records,
`NX=NY=50, NZ=9`, `IRTYPE=1`, `IWFCOD=1`, `LCALGRD=T`, `NSSTA=1`, `NUSTA=1`, `NPSTA=0`.

Parsing that file into the explicit arrays and serialising it back produces a **byte-identical**
image (`cmp` clean, output sha256 equal to the frozen digest). The same file is then read back by
three independent consumers:

- the writer's own fail-closed parser (self-consistency only);
- an **independent Python record walk** in `tests/verify_real_reference_roundtrip.py`
  (framing, 96-byte declaration, 1,114×132-byte comments, 176-byte controls, the ten static records
  with zero clock words, the OUTHR order, the packed time labels, contiguity, and an aligned XMISS
  count per label) whose inventory must equal the writer's `check` report field by field;
- the **P7 decoder** `native_cpp/modules/p7_calpuff/reference/original_decode_calmet_hourly.py`
  (read-only), which must report `PASS_ORIGINAL_CALMET_EXPLICIT_TIME_AND_FIELD_LAYOUT` on the
  re-serialised file and produce a report **identical** to the one it produces for the frozen
  original, including its own sha256 of the file it read;
- the **E22 contract reader** `cteas::p6e::read_calmet_dat21` (P6E module, read-only include), which
  must parse the re-serialised real case and agree on all 6,435 field records cell by cell
  (bitwise float comparison), and vice versa (the P6F parser must read what the E22 writer produces).

Observation about the frozen artifact: the hourly field values of `met.dat` contain **no** `XMISS`
sentinel (0/15,372,500 real and 0/715,000 integer values of 16,087,500 stored cells), so the
sentinel path of the inventory is exercised only by the synthetic fixtures. This says nothing about
precipitation: the P6C record of the same case already documents `IPCODE=9999` 143/143 in `SURF.DAT`
and `NPSTA=0`.

## Gate

```
python3 scripts/local_gate.py --track p6f     # LOCAL_GATE_P6F=PASS
```

Delivered as PR [#26](https://github.com/RRain001/CTEAS/pull/26) (base `main`); the script writes
its own machine-readable evidence under `$CTEAS_GATE_ROOT/evidence/p6f-<sha12>.json`.

The gate configures, builds (`g++-12`, `-Wall -Wextra -Wpedantic -Werror`) and runs the module's
four CTest targets:

| CTest | What it proves |
|---|---|
| `p6f_calmet_dat21_writer_unit` | explicit arrays → records → read back, per-field bit equality, the calendar/time helpers over 1600–2400, XMISS preservation, and the ~50-case fail-closed matrix |
| `p6f_calmet_dat21_synthetic_cli_roundtrip` | CLI end to end on four synthetic variants (including `NSSTA=0` and `NPSTA=3`), with an independent Python walk of every emitted record plus CLI-level fail-closed options |
| `p6f_calmet_dat21_real_reference_round_trip` | the frozen byte identity + SHA-256 pin + independent inventory agreement + P7 decoder cross-read + six fail-closed negatives |
| `p6f_calmet_dat21_e22_contract_cross_read` | E22 reader on the re-serialised frozen case (6,435 records) and P6F reading an E22-written file |

Both real-case tests are registered fail-closed: `CMakeLists.txt` raises a configure error if the
frozen reference file or the P7 decoder is absent, so the byte-identity proof can never be silently
skipped.

## Hard limits and open dependencies

- **This is a serialiser, not a physics result.** Byte identity proves the record layout, time
  semantics, units and masks. It does **not** show that reconstructed 2022 fields equal the original
  CALMET values; the writer stores whatever value words the caller supplies, including wrong ones.
  The P6F acceptance clause "物理值正确" therefore remains **open** and belongs to P6G, which must
  compare the P6D/P6E chains' fields with the frozen original under the pre-registered tolerances.
  Status of this work: writer implemented, layout + byte identity PASS, **field values NOT accepted**.
- Supported output modes: `IRTYPE=1` with `IWFCOD=1` and `NPSTA>=0`. `IRTYPE=0` (wind-only file) and
  `NPSTA=-1` (MM5 precipitation) are described by the layout tables but refused by the writer.
- The projection block is only byte-preserved and typographically validated; only the frozen case's
  UTM/WGS-84 block (`UTM`, `WGS-84`, `02-21-2003`, `N   `, `IUTMZN=50`) has been exercised end to end.
- `ILANDU` values are not range-checked against `NLU` (the frozen case itself carries category 20
  with `NLU=14`), so land-use validity remains a caller/upstream responsibility.
- The E22 header is consumed only as an independent decoder through an explicit include path; the P6F
  library and CLI include no P6E/P6D/P7 header. If E22 changes, the cross-read test must be re-run.
- The writer does not close P6F as a stage milestone: it closes the writer/layout half of it.
