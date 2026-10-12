# CTEAS CI runner-pool routing migration (2026-10-12)

Status: **CONFIGURATION_READY / SCIENTIFIC_CI_NOT_YET_REPLAYED**.
Base: `main`. Runner pool deployment and 13/13 concurrent independent infrastructure acceptance are recorded in draft PR #34. This document does **not** claim scientific model equivalence or successful replay of any routed workflow.

## Current runner pool contracts

- Light: `[cteas-managed, cteas-light, Linux, X64]` → r04–r11 (2 CPU quota, 3 GiB MemoryMax).
- Compute: `[cteas-managed, cteas-compute, Linux, X64]` → r12–r14 (6 CPU quota, 12 GiB MemoryMax).
- Reserve: `[cteas-managed, cteas-reserve, Linux, X64]` → r15–r16 (4 CPU quota, 6 GiB MemoryMax). No existing heavy workflow is routed here because its 6 GiB ceiling has not been established as sufficient.
- Unlike the three original runner registrations, **new managed runners omit the default `self-hosted` label**. Never include `self-hosted` in their `runs-on` clauses.
- Original runner `lhy-desktop`, `lhy-desktop-2`, `lhy-desktop-3` jobs are unchanged. Do not repoint dedicated P6D, P6E or P7 jobs during live development.

## Changes: old `[self-hosted, Linux, X64]` → new pool

| Pool | Workflow files |
|---|---|
| light | `p6a-calmet-contracts.yml`, `fortran-reference-p1-p2.yml`, `p4-read62-scoped.yml`, `cpp-input-contracts.yml`, `p5-smerge-scoped.yml`, `p6c-comp-source-inspection.yml`, `p4-hefei-2022-data-capture.yml`, `p5-isd-lite-independent-research.yml`, `p5-native-surf-parity.yml`, `p5-hefei-2022-data-capture.yml` |
| compute | `p6c-diagi-station-parity.yml`, `p6c-calmet-terset-parity.yml`, `p6c-readge-fortran-parity.yml`, `p1-terrel-reference-compare.yml`, `p2-gen-reference-compare.yml`, `p3-makegeo-reference-compare.yml`, `p5-real-multiyear-reference.yml`, `p4-read62-fortran-parity.yml`, `p6b-calmet-fortran-reference.yml`, `p5-isd-lite-multistation-wind-audit.yml`, `p4-p5-original-calmet-integration.yml` |

Total: **21 existing workflow files**. Job trigger conditions, numeric thresholds, `permissions`, `concurrency`, model sources and build types remain unchanged.

## Managed runner compatibility changes

1. Several legacy workflows attempted `sudo apt-get install libgdal-dev`. The managed runner accounts deliberately have no sudo. Replace this fallback with a fail-closed GDAL preflight; prerequisite `pkg-config --exists gdal` must already be true on the server. No host package is installed by a CI job.
2. Workflows that explicitly run `python3 -m pip install` now create a per-runner temporary Python venv and prepend its `bin` to `GITHUB_PATH`. This avoids attempting global pip writes as the unprivileged runner user. Host must provide `python3 -m venv` and pip bootstrap.
3. Legacy literal `/tmp/...` references in the workflows being migrated are replaced with `${{ runner.temp }}/...`. This isolates build/evidence directories across distinct Runner instances (while preserving the relative names and original scientific step commands).
4. The shared action `.github/actions/cteas-linux-toolchain/action.yml` is unchanged. It selects modern `g++` and `gfortran` without modifying system alternatives.

## Deliberately out of scope

- `cteas-selfhosted-smoke.yml`: PR #34 already routes its normal smoke to `cteas-light` and adds optional 13-runner infrastructure acceptance. Do **not** introduce a conflicting duplicate patch.
- Dedicated P6D/P6E/P7 workflows requiring named runner labels and `RUNNER_NAME` fail-closed checks remain on original runner #1/#2/#3.
- Existing `scripts/local_gate.py` runs directly by SSH and is **not automatically dispatched to GitHub Runner pools**.
- Existing workflow-specific hardcoded parallelism, package dependency requirements and test timeouts are not claimed optimized by this label migration. Global `/tmp` paths inside separately invoked project scripts, if any, require a separate audit.

## Verification and adoption gates

- Existing managed pool infrastructure acceptance: [PR #34](https://github.com/RRain001/CTEAS/pull/34), [13-job real concurrency success](https://github.com/RRain001/CTEAS/actions/runs/38189867178); this establishes runner/compilers/cgroups/network, **not CTEAS scientific workload**.
- This branch changes workflow YAML only and this audit note; no scientific source files. All commits have `[skip ci]` to avoid simultaneously triggering many PR-scoped scientific workflows. Skipped checks **must not** be presented as passing.
- Recommend review and merge PR #34 first (shared smoke routing), then this branch after updating from the new base. Their changed workflow file sets are disjoint.
- After adoption, explicitly replay representative jobs, **one per category initially**, and collect job ID, assigned `RUNNER_NAME`, exit status, CTest/numeric outcomes, memory peak and OOM/throttling counters. Suggested staging: P6A/CPP contracts (light), P1/P2 original parity and P6C selected original parity (compute), then P6B/P4–P5 longer original benchmarks after GDAL/Python prerequisites are proven.
- Fail closed on missing Python/GDAL dependencies, memory limit events, or changed numerical parity results; increase pool resources or remediate infrastructure explicitly, never loosen model tolerances or label a skipped job as PASS.
- If any migrated CI path fails, revert this workflow-routing PR or temporarily restore the original individual job's `runs-on` via a separate reviewed change. Do not alter source/physics to mask an infrastructure error.
