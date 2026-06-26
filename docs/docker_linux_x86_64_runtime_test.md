# Docker Linux x86_64 Runtime Test

执行日期：2026-06-26

项目根目录：

- `/Users/rain/CTEAS/tutorial`

测试脚本：

- `scripts/docker_network_diagnose.sh`
- `scripts/test_linux_x86_64_runtime_in_docker.sh`

## 后续收尾

同日后续又做了一轮非 Docker 收尾清理，本轮没有重跑 Docker runtime，只处理：

1. `project/cases/smoke5d_docker_test` 中允许删除的旧历史目录
2. 本地 `.venv` 坏链修复与重建

收尾结果：

- 已清理：
  - `project/cases/smoke5d_docker_test/deliverables/calmet`
  - `project/cases/smoke5d_docker_test/deliverables/calpuff`
- 未清理：
  - `project/cases/smoke5d_docker_test/runs/20260514_152231_calmet`
  - `project/cases/smoke5d_docker_test/runs/20260514_152537_calpuff`

说明：

- 上述 `runs/*_calmet` / `runs/*_calpuff` 是历史运行记录目录，不在本轮允许删除范围内
- 本轮没有重跑 `./scripts/test_linux_x86_64_runtime_in_docker.sh`
- 下文 Docker runtime 结果仍对应前一轮已经通过的容器实跑

## 本轮修复范围

本轮按最新 `plan.md` 完成了以下修复：

1. 修复 native logical name 到 physical binary 的正式映射：
   - `met -> calmet.x`
   - `puff -> calpuff.x`
2. 新增 runner 映射检查脚本：
   - `scripts/check_native_runner_mapping.py`
3. 更新 `scripts/check_native_binaries.py` 输出，明确显示 logical -> physical 映射结果。
4. 移除 Docker 测试脚本中的临时 symlink workaround：
   - 不再创建 `bin/linux-x86_64/met.x`
   - 不再创建 `bin/linux-x86_64/puff.x`
5. 修复 `run_met` 对旧坏链输出的处理：
   - 不再沿用旧的 `run/met/calmet.lst -> ../../deliverables/calmet/calmet.lst`
   - 运行时直接在 `run/met/` 生成 `calmet.lst` / `calmet.dat`
6. 修复 `prepare_puff` 的 `METDAT` 口径：
   - `../../deliverables/met/calmet.dat`
7. 修复 `run_puff` 对旧坏链输出的处理：
   - 运行时直接在 `run/puff/` 生成 `calpuff.con`
   - 同步复制到 `deliverables/puff/`
8. 修复 Docker 脚本对本地镜像存在性的探测方式：
   - 不再用 `docker image inspect`
   - 改为用 `docker images` 判断本地 `debian:12` 是否存在

## 执行前检查

已执行：

- `git status --short`
- `python3 -m compileall engine scripts`
- `PYTHONPATH=.venv/lib/python3.14/site-packages python3 scripts/check_current_case.py`
- `PYTHONPATH=.venv/lib/python3.14/site-packages python3 scripts/check_native_binaries.py --platform linux-x86_64`
- `PYTHONPATH=.venv/lib/python3.14/site-packages python3 scripts/check_native_runner_mapping.py --platform linux-x86_64`
- `PYTHONPATH=.venv/lib/python3.14/site-packages python3 -m engine.run_met.prepare_met --case smoke5d_docker_test`
- `PYTHONPATH=.venv/lib/python3.14/site-packages python3 -m engine.run_puff.prepare_puff --case smoke5d_docker_test`
- `bash -n scripts/test_linux_x86_64_runtime_in_docker.sh`
- `bash -n scripts/docker_network_diagnose.sh`

补充说明：

- 早先宿主机检查阶段曾遇到 `.venv/bin/python` 坏链
- 后续收尾轮已执行：
  - `rm -rf .venv`
  - `python3 -m venv .venv`
  - `./.venv/bin/python -m pip install --upgrade pip`
  - `./.venv/bin/python -m pip install -r requirements.txt`
- 当前宿主机检查已恢复使用：
  - `./.venv/bin/python`

## 本地静态检查结果

后续收尾轮宿主机检查结果：

- `./.venv/bin/python -m compileall engine scripts`：通过
- `./.venv/bin/python scripts/check_current_case.py`：通过
- `./.venv/bin/python scripts/check_native_binaries.py --platform linux-x86_64`：通过
- `./.venv/bin/python scripts/check_native_runner_mapping.py --platform linux-x86_64`：通过
- `bash -n scripts/test_linux_x86_64_runtime_in_docker.sh`：通过
- `bash -n scripts/docker_network_diagnose.sh`：通过
- `MPLCONFIGDIR=/private/tmp/mpl QT_QPA_PLATFORM=offscreen ./.venv/bin/python scripts/test_gui_does_not_switch_current_case.py`：通过

GUI current-case 检查输出：

```text
before: smoke5d
after: smoke5d
PASS: MainWindow initialization did not switch current case
```

### current case

结果：

- `current_case_name: smoke5d`
- `current_case_dir: /Users/rain/CTEAS/tutorial/project/cases/smoke5d`

### native binary presence

结果：

- `terrel -> terrel.x OK`
- `ctgproc -> ctgproc.x OK`
- `makegeo -> makegeo.x OK`
- `read62 -> read62.x OK`
- `smerge -> smerge.x OK`
- `met -> calmet.x OK`
- `puff -> calpuff.x OK`

### runner mapping

结果：

- `met resolves to /Users/rain/CTEAS/tutorial/bin/linux-x86_64/calmet.x`
- `puff resolves to /Users/rain/CTEAS/tutorial/bin/linux-x86_64/calpuff.x`
- `runner mapping OK`

### prepare 输出核对

`prepare_met` 结果：

- `run/met/calmet.inp` 正常生成

`prepare_puff` 结果：

- `run/puff/calpuff.inp` 正常生成
- `METDAT` 已渲染为：

```text
../../deliverables/met/calmet.dat
```

## Docker linux/amd64 runtime

实际执行：

```bash
./scripts/test_linux_x86_64_runtime_in_docker.sh
```

容器内关键信息：

- `uname -m = x86_64`
- `platform_key = linux-x86_64`

Docker 脚本当前状态：

- 已移除 `met.x` / `puff.x` symlink workaround
- 运行过程中不再创建：
  - `bin/linux-x86_64/met.x`
  - `bin/linux-x86_64/puff.x`

## 运行结果

### Docker 平台与依赖

1. 是否使用代理：否
2. Docker 平台：`linux/amd64`
3. apt 是否成功：成功
4. pip 是否成功：成功

### case 核对

1. 当前 case 是否仍为 `smoke5d`：是
2. 测试 case 是否为 `smoke5d_docker_test`：是

### READ62

结果：

- `run_read62` 成功进入 `read62.x`
- 返回：
  - `成功: 1`
  - `失败: 0`

分类：

- 通过

### MET

结果：

- `run_met` 成功调用 `calmet.x`
- `run/met/calmet.inp` 正常使用
- `run/met/calmet.lst` 正常生成
- `run/met/calmet.dat` 正常生成

运行期打印：

```text
[INFO] 工作目录: /work/project/cases/smoke5d_docker_test/run/met
[INFO] 风场数据: /work/project/cases/smoke5d_docker_test/run/met/calmet.dat
[INFO] 运行记录: /work/project/cases/smoke5d_docker_test/run/met/calmet.lst
```

检查结果：

- `运行记录生成: True`
- `风场文件生成: True`
- `[RESULT] 诊断风场计算完毕`

分类：

- 通过

### PUFF

结果：

- `run_puff` 成功调用 `calpuff.x`
- `calpuff.inp` 中 `METDAT` 已指向：
  - `/work/project/cases/smoke5d_docker_test/deliverables/met/calmet.dat`
- `run/puff/calpuff.lst` 正常生成
- `run/puff/calpuff.con` 正常生成

运行期打印：

```text
[INFO] 工作目录: /work/project/cases/smoke5d_docker_test/run/puff
[INFO] 气象数据: /work/project/cases/smoke5d_docker_test/deliverables/met/calmet.dat
[INFO] 运行记录: /work/project/cases/smoke5d_docker_test/run/puff/calpuff.lst
[INFO] 浓度文件: /work/project/cases/smoke5d_docker_test/run/puff/calpuff.con
```

检查结果：

- `运行记录生成: True`
- `浓度文件生成: True`
- `[RESULT] 烟团模型计算完毕`

分类：

- 通过

## 结果汇总

1. Docker 脚本是否已移除 symlink workaround：是
2. `met` 是否正确映射到 `calmet.x`：是
3. `puff` 是否正确映射到 `calpuff.x`：是
4. MET 是否生成/读取 `calmet.inp`、`calmet.lst`、`calmet.dat`：是
5. PUFF 的 `METDAT` 是否指向 `../../deliverables/met/calmet.dat`：是
6. READ62/MET/PUFF 运行结果：全部通过
7. 当前失败点如果还有：无

## inspect outputs 结果

容器内最终确认存在：

- `project/cases/smoke5d_docker_test/run/met/calmet.inp`
- `project/cases/smoke5d_docker_test/run/met/calmet.lst`
- `project/cases/smoke5d_docker_test/run/met/calmet.dat`
- `project/cases/smoke5d_docker_test/run/puff/calpuff.inp`
- `project/cases/smoke5d_docker_test/run/puff/calpuff.lst`
- `project/cases/smoke5d_docker_test/run/puff/calpuff.con`
- `project/cases/smoke5d_docker_test/deliverables/met/calmet.dat`
- `project/cases/smoke5d_docker_test/deliverables/met/calmet.lst`
- `project/cases/smoke5d_docker_test/deliverables/puff/calpuff.con`
- `project/cases/smoke5d_docker_test/deliverables/puff/calpuff.lst`

## 残留说明

当前运行链路已经不依赖旧命名目录：

- MET 正式工作文件：`run/met/calmet.*`
- PUFF 正式 `METDAT`：`deliverables/met/calmet.dat`
- PUFF 正式输出：`run/puff/calpuff.*` 与 `deliverables/puff/calpuff.*`

后续收尾轮已进一步清理：

- `deliverables/calmet/`
- `deliverables/calpuff/`

因此当前保留的旧命名残留只剩不在本轮删除范围内的历史 `runs/*_calmet` / `runs/*_calpuff` 目录。

## 本轮修改文件

- `engine/native/registry.py`
- `engine/core/platform_utils.py`
- `engine/run_met/run_met.py`
- `engine/run_puff/prepare_puff.py`
- `engine/run_puff/run_puff.py`
- `scripts/check_native_binaries.py`
- `scripts/check_native_runner_mapping.py`
- `scripts/test_linux_x86_64_runtime_in_docker.sh`
- `docs/docker_linux_x86_64_runtime_test.md`
