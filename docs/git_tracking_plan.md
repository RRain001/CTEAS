# Git Tracking Plan

## Current Git State

- `tutorial/` 已经是 Git 仓库。
- 当前工作区不是干净状态，存在大量既有修改与未跟踪文件。
- 本轮不会回退这些既有修改。

## Track In Git

建议纳入 Git 的内容：

- `engine/`
- `scripts/`
- `docs/`
- `docker/`
- `calpuff-cli`
- `requirements.txt`
- `requirements-docker-runtime.txt`
- `pyproject.toml`
- `AGENTS.md`
- `plan.md`
- `.gitignore`
- `.gitattributes`
- `project/current_case.yaml`

## Ignore In Git

通过 `.gitignore` 忽略：

- `.venv/`、`.venv-docker/`、`__pycache__/`
- `project/current`
- `project/cases/*/run/`
- `project/cases/*/runs/`
- `project/cases/*/deliverables/`
- `project/cases/*/logs/`
- `project/cases/smoke5d_docker_test/`
- `project/cases/*/geo/`
- `project/cases/*/met/`
- 大型地形/土地利用数据目录 `calpuff_terrain_and_landuse_data/`
- `docs/refactor_logs/removed_native_binaries_backup/`
- `bin/linux-x86_64/*.x`

## Native Binary Decision

- `bin/linux-x86_64/*.x` 体量差异很大。
- 当前最大的是 `bin/linux-x86_64/calpuff.x`，约 `545M`。
- 因此当前不建议把 `bin/linux-x86_64/*.x` 纳入普通 Git 跟踪。
- 推荐只保留目录结构，二进制通过本地备份、发布包或源码重建恢复。
- 如果后续确实需要跟踪，优先考虑 Git LFS。

## Case Data Decision

`project/cases/smoke5d` 含有必要配置，也含有大量中间结果和气象文件。

当前建议：

- 可以纳入：
  - `project/cases/smoke5d/case.yaml`
  - `project/cases/smoke5d/grid/`
- 暂不纳入，需人工确认：
  - `project/cases/smoke5d/geo/` 中哪些小型模板输入必须保留
  - `project/cases/smoke5d/met/` 中哪些最小气象样例必须保留
- 明确不纳入：
  - `project/cases/*/run/`
  - `project/cases/*/runs/`
  - `project/cases/*/deliverables/`
  - `project/cases/*/logs/`

## Files Requiring Manual Confirmation

- `project/cases/smoke5d/geo/` 下哪些 `.dat/.grd` 是最小测试必需输入。
- `project/cases/smoke5d/met/` 下哪些 CSV/upperair 文件需要保留为最小 smoke case。
- `bin/linux-x86_64/*.x` 是否需要 Git LFS。

## Git LFS Outlook

- 由于 `calmet.x`、`terrel.x`、尤其 `calpuff.x` 体量很大，如果未来要推远程仓库，建议改用 Git LFS 或发布附件。
