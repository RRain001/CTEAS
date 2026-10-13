# P7 output chain (original WROUT1 / AVET) audit — 2026-10-12

Selected frozen case: `hefei5km_2022_reference_case_20261011`, `METFM=1, MWET=0`. This document is an **audit of original Fortran output requirements, not an assertion that native C++ WROUT1 is implemented**.

## Case-specific AVET behavior

The active control has `AVET=60` minutes and `PGTIME=60` minutes. Therefore the PG sigma-y time-scaling factor `(AVET/PGTIME)^0.2` is exactly **1** for this case. A blanket claim that no output averaging exists is **not** justified: `AVET` is the PG dispersion averaging-time parameter, whereas the model still must accumulate hourly receptor concentration in the original output chronology. Any other deck (`AVET != PGTIME`) requires its own numerical evaluation.

## WROUT1 / CONC.DAT gap is countable

Frozen original `puff.con`: 1,896,056 bytes; SHA256 `0d51bdb68ff1a5798048bc41067107d3ffe10dc6222f25336df856bcbf48ba10`. There are **3,655** Fortran sequential-unformatted records. The 143 hourly periods contribute **429** data records (period / TOTAL source descriptor / H-3 REAL4 field). **Direct binary decoding in GitHub-hosted original-execution-order CI #38379037656** established that the first two records are the 96-byte version record and four-byte count **3218**, followed immediately by **3,218 embedded 132-byte control/comment records**. This leaves only **six as-yet-unclassified non-hourly records** beyond the comment prefix and 429 data records. The earlier estimate that *3,224 distinct header records had to be reimplemented* was inaccurate: 3,218 are an embedded-text stream, not distinct physical subroutines or metadata tables.

The existing C++ `ConcFileWriter` already writes version/count and an arbitrary vector of 132-byte comment records; **it does not yet independently reconstruct the original 3,218 embedded lines**. Do not copy reference `puff.con` header bytes. The original `puff.inp` has a different text-line count, so reproducing the comment stream requires auditing original Fortran input echo/preprocessing, not merely calling `splitlines()` on `puff.inp`. The six residual records still need original `WROUT1` statement mapping and native construction.

The new `reference/audit_vendor_wrout1_output.py` reads the *real frozen* binary file as Fortran sequential-unformatted records (checking marker/payload/trailer size), validates its SHA256, counts and histograms every record size, and confirms the selected `AVET=PGTIME` branch. It saves first-record byte offsets and sizes as evidence. A separate CTest and fast P7 Runner job gate verify these invariants. Even on PASS, this is **reference output characterization only**.

Next required work is mapping each of the six remaining vendor WROUT1 metadata records and the original 3,218-line embedded-text generation to Fortran write statements and its required run/source/grid metadata, implementing an independently produced C++ record stream (not copying original frozen headers), and comparing every 143-hour concentration value and record. `CONC.DAT` formatting must preserve original record sequencing and REAL*4 storage. The complete model must also keep its per-hour `POINTS1→COMP` state authoritative and satisfy 1h/6h/143h puff-step DGM parity.

**Boundary:** `WROUT1` parity, C++ concentration-value parity, and a complete original-ordered C++ driver are **not implemented / not accepted** by this audit. `MWET=0` means wet removal is outside this frozen-case acceptance.
