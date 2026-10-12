# P7 — 16 self-hosted runner lanes (2026-10-12)

At this commit, P7 CI is configured for **16 initially independent, dispatchable jobs**: **14 source-parity matrix lanes** + **one REAL*4 precision/kind audit** + **one COMP source-order, internal coordinate, DGM state, and WROUT1 reference gate**.  A distinct extra `native-first-six-hours` job regenerates the original DGM and checks the 1h/6h/143h oracle-fed post-wind motion state; it no longer waits for `original-execution-order` because its frozen control/MET.DAT/vendor SHA and reference output checks are independent. GitHub may still queue any job if its matching runner is offline or occupied. **Configuring 16 jobs is not proof that 16 runner instances actually ran concurrently.**

## Per-job duties (33 existing mandatory source-parity tests, no gaps)

1. `release-inject`: `cteas_p7o_release_scheduler_contract`; `cteas_p7o_source_parity`.
2. `step-bookkeeping`: `cteas_p7p_step_bookkeeping_contract`; `cteas_p7p_source_parity`.
3. `advection`: `cteas_p7v_advection_contract`; `cteas_p7v_source_parity`.
4. `dispersion`: `cteas_p7d_dispersion_contract`; `cteas_p7d_sigma_source_parity`.
5. `plume-rise`: `cteas_p7n_plume_rise_contract`; `cteas_p7n_source_parity`.
6. `sigma-update`: `cteas_p7q_sigma_update_contract`; `cteas_p7q_source_parity`.
7. `terrain-constraints`: `cteas_p7r_terrain_constraints_contract`; `cteas_p7r_source_parity`.
8. `terrain-relief`: `cteas_p7s_terrain_state_contract`; `cteas_p7s_source_parity`.
9. `valley-width`: `cteas_p7t_valley_width_contract`; `cteas_p7t_source_parity`.
10. `receptor-sampling`: `cteas_p7e_receptor_sampling_conc_contract`; `cteas_p7e_source_parity`.
11. `vertical-coupling`: `cteas_p7h_vertical_coupling_contract`; `cteas_p7h_source_parity`.
12. `ctsg-kernels`: `cteas_p7i_ctsg_support_contract`; `cteas_p7i_source_parity`; `cteas_p7m_ctsg_kernel_contract`; `cteas_p7m_source_parity`.
13. `dry-removal`: `cteas_p7f_dry_deposition_contract`; `cteas_p7f_locked_case_dry_deposition`; `cteas_p7f_drydep_source_parity`.
14. `wind-step-output`: `cteas_p7w_rise_wind_contract`; `cteas_p7w_source_parity`; `cteas_p7x_step_integration_contract`; `cteas_p7g_conc_structure`.

Plus `precision-kind-audit` and `original-execution-order`. The DGM full-run is a separate *scientific* job, not a dummy keep-alive. All physical matrix lanes require their tests to be **registered and actually run**, with `P7_STAGE_BEGIN` / `P7_STAGE_PASS` evidence; Fortran 10+ is mandatory. Each job prints `RUNNER_NAME` and `hostname`, so map real names to actual physical hosts using logs, not the number of registered instances. Do not silently count cancelled, queued, or unregistered tests as passed.

## Host admission

`strategy.max-parallel=16` removes the previous `4`-job scheduler ceiling. The source-parity matrix uses a shared per-host `flock` pool in `/tmp/cteas-p7-parity-host-slots` to bound actual physical **compiling / Fortran source parity to four tasks per host** while holding 16 independent Runner jobs eligible. This reduces CPU/RAM thrash when all runners happen to live on `lhy-desktop`. The capture and complete P7 test may have separate CPU pressure; observe runner topology and system usage before increasing the host compute limit. A valid job will log `P7_HOST_RESOURCE_ADMITTED=PASS`. No P6 workflow or vendor Fortran source modified.

## Scientific scope

33/33 isolated P7 kernel regressions passed previously at the earlier baseline [CI #38231475547](https://github.com/RRain001/CTEAS/actions/runs/38231475547), but the **newly sharded CI must pass again** and prove actual runner utilization. The first-hour native position/distance/age state passed direct DGM 1,275-record comparison at [CI #38239376838](https://github.com/RRain001/CTEAS/actions/runs/38239376838). The new 6h and 143h validations are still **oracle-fed post-wind** and at hour boundaries re-seed from original. Autonomous `SWAP/ROLLDN`, full original-ordered COMP meteorology and output concentration, WROUT1/AVET record generation, and exact C++ `puff.con` field values **remain unaccepted**. PR #41 remains Draft until all pass.
