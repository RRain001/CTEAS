# MET/PUFF Rename Inventory

## Scope

本轮按 `plan.md` 完成了以下命名统一：

- Python 运行包：
  - `engine/run_calmet/ -> engine/run_met/`
  - `engine/run_calpuff/ -> engine/run_puff/`
- Python 入口文件：
  - `prepare_calmet.py -> prepare_met.py`
  - `run_calmet.py -> run_met.py`
  - `prepare_calpuff.py -> prepare_puff.py`
  - `run_calpuff.py -> run_puff.py`
- 逻辑 stage 名称：
  - `calmet -> met`
  - `calpuff -> puff`
- case 配置键：
  - `calmet:` -> `met:`
  - `calpuff:` -> `puff:`
  - `run_calmet_dir -> run_met_dir`
  - `run_calpuff_dir -> run_puff_dir`
- 运行目录：
  - `run/calmet -> run/met`
  - `run/calpuff -> run/puff`
  - `deliverables/calmet -> deliverables/met`
  - `deliverables/calpuff -> deliverables/puff`

## Files Changed

- 运行链核心：
  - `engine/native/registry.py`
  - `engine/core/paths.py`
  - `engine/orchestrator/cli.py`
  - `engine/run_met/*`
  - `engine/run_puff/*`
- case/schema：
  - `engine/config/case.schema.yaml`
  - `engine/validators/case_schema.py`
  - `engine/preprocess/bin/new_case.py`
  - `engine/preprocess/bin/run_preprocessing_pipeline.py`
  - `project/cases/smoke5d/case.yaml`
  - `project/cases/smoke5d_docker_test/case.yaml`
- GUI/脚本：
  - `engine/gui/workbench.py`
  - `engine/gui/app.py`
  - `scripts/check_paths.py`
  - `scripts/test_native_backend.sh`
  - `scripts/test_linux_x86_64_runtime_in_docker.sh`
- 文档与项目元数据：
  - `docs/docker_linux_x86_64_runtime_test.md`
  - `docs/git_tracking_plan.md`
  - `docs/path_inventory.md`
  - `docs/project_structure_for_chatgpt.md`
  - `docs/regression_test_after_path_refactor.md`
  - `engine/run_met/README.md`
  - `engine/run_puff/README.md`
  - `pyproject.toml`

## Allowed Residual Matches

以下旧名保留是有意为之，不属于本轮遗漏：

- `allowed_upstream_binary_name`
  - `engine/native/registry.py` 中 `binary="calmet"` / `binary="calpuff"`
  - `scripts/build_native_linux_x86_64.sh`
  - `scripts/build_native_linux_arm64.sh`
  - `scripts/test_native_backend.sh` 中 `met -> calmet`、`puff -> calpuff` 的物理映射
  - 原因：本轮只改逻辑名，不重命名底层 `.x`

- `allowed_template_or_output_filename`
  - `engine/templates/calmet.inp`
  - `engine/templates/calpuff.inp`
  - `run/met/calmet.inp`、`run/met/calmet.dat`、`run/puff/calpuff.inp`、`run/puff/calpuff.con` 这类文件名引用
  - 原因：这些是上游控制文件/输出文件名接口，不是项目逻辑 stage 名

- `allowed_history_note`
  - `engine/run_met/log.md`
  - `engine/run_met/log copy.md`
  - `engine/run_met/并行计算的尝试.md`
  - `docs/refactor_logs/*`
  - 原因：这些是历史记录，不作为当前运行入口

- `allowed_binary_inventory_doc`
  - `docs/native_binaries.md`
  - `docs/native_binary_cleanup_report.md`
  - 原因：文档记录的是实际物理二进制 `calmet.x` / `calpuff.x`

- `allowed_backward_alias`
  - `engine/orchestrator/cli.py` 中临时保留 `calmet -> met`、`calpuff -> puff`
  - 原因：兼容旧 CLI 调用，同时输出 deprecated 提示

## Remaining Actionable Matches

- 当前没有发现需要继续修改的 `run_calmet` / `run_calpuff` 正式运行入口引用。
- 当前没有发现 GUI stage 显示仍使用 `CALMET` / `CALPUFF` 的位置。
- 当前没有发现 `case.yaml` 仍使用 `calmet` / `calpuff` 作为运行层配置键的活跃案例配置。

## Verification Basis

- 改名前全量命中清单：
  - `docs/refactor_logs/before_met_puff_rename_matches.txt`
- 改名后残留清单：
  - `docs/refactor_logs/after_met_puff_rename_remaining_matches.txt`
- 残留项已经按上面的 `allowed_*` 类别人工解释。
