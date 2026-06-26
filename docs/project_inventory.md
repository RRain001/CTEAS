# CTEAS Tutorial Project Inventory

## 当前项目入口

- CLI 总入口：`/Users/rain/CTEAS/tutorial/calpuff-cli`
- 编排入口：`/Users/rain/CTEAS/tutorial/engine/orchestrator/cli.py`
- GUI 入口：`/Users/rain/CTEAS/tutorial/engine/pyside_gui.py`
- GUI 主模块：
  - `/Users/rain/CTEAS/tutorial/engine/gui/app.py`
  - `/Users/rain/CTEAS/tutorial/engine/gui/workbench.py`
- 已废弃入口：`/Users/rain/CTEAS/tutorial/engine/gui/__main__.py`
  - 当前只提示 Web GUI 已移除。

## 项目结构

- `engine/`
  - Python GUI、业务逻辑、预处理、后处理、模板、资源、native 源码和旧二进制
- `project/cases/`
  - 示例工程
- `calpuff_terrain_and_landuse_data/`
  - 地形和土地利用基础数据
- `bin/`
  - 本轮新增的平台化二进制目录
- `docker/fortran/`
  - 本轮新增的 Docker 编译环境
- `scripts/`
  - 本轮新增的 native 构建和测试脚本

## 代码与配置盘点

### Python 入口脚本

- `/Users/rain/CTEAS/tutorial/calpuff-cli`
- `/Users/rain/CTEAS/tutorial/engine/pyside_gui.py`
- `/Users/rain/CTEAS/tutorial/engine/orchestrator/cli.py`

### GUI 主脚本

- `/Users/rain/CTEAS/tutorial/engine/gui/app.py`
- `/Users/rain/CTEAS/tutorial/engine/gui/workbench.py`

### YAML / JSON / INI

- YAML
  - `/Users/rain/CTEAS/tutorial/engine/config/case.schema.yaml`
  - `/Users/rain/CTEAS/tutorial/project/cases/*/case.yaml`
- JSON
  - 未发现项目级运行状态 JSON；当前 case 选择依赖 `project/current` 软链接
- INI
  - 未发现 `.ini` 配置

### shell 脚本

- `/Users/rain/CTEAS/tutorial/bootstrap_venv.sh`
- `/Users/rain/CTEAS/tutorial/calpuff-cli`

### native 源码

- 主要为 Fortran：
  - `*.for`
  - `*.f`
- 当前未发现用于 native 主程序构建的 C / C++ 源文件

### 依赖文件

- 已有：`/Users/rain/CTEAS/tutorial/requirements.txt`
- 未发现：
  - `environment.yml`
  - `pyproject.toml`
  - `setup.py`

### 数据文件和示例工程

- 示例 case：
  - `/Users/rain/CTEAS/tutorial/project/cases/test`
  - `/Users/rain/CTEAS/tutorial/project/cases/test2`
  - `/Users/rain/CTEAS/tutorial/project/cases/123`
- 地形数据：
  - `/Users/rain/CTEAS/tutorial/calpuff_terrain_and_landuse_data/china_srtm`
- 土地利用数据：
  - `/Users/rain/CTEAS/tutorial/calpuff_terrain_and_landuse_data/eurasia_asia_landuse/eausgs2_0la.img`

## 当前 Python 依赖来源

当前依赖由 `requirements.txt` 提供，按实际 import 补齐后包含：

- `PySide6`
- `PyYAML`
- `numpy`
- `pandas`
- `matplotlib`
- `imageio-ffmpeg`

## 当前 `.x` 文件位置

- 旧运行目录：`/Users/rain/CTEAS/tutorial/engine/vendor/bin/*.x`
- 源码目录自带副本：`/Users/rain/CTEAS/tutorial/engine/vendor/src/*/*.x`
- 本轮新增统一目录：
  - `/Users/rain/CTEAS/tutorial/bin/linux-x86_64/`
  - `/Users/rain/CTEAS/tutorial/bin/linux-arm64/`
  - `/Users/rain/CTEAS/tutorial/bin/windows-x86_64/`

## `.x` 对应功能推测

- `terrel.x`：地形预处理
- `ctgproc.x`：土地利用预处理
- `makegeo.x`：地理场合并
- `read62.x`：高空气象转换
- `smerge.x`：地面气象合并
- `calmet.x`：诊断风场
- `calpuff.x`：烟团扩散主求解

## 是否存在源码可重新编译

存在，源码位于 `/Users/rain/CTEAS/tutorial/engine/vendor/src/`。

## 当前路径是否写死

仍存在两类问题：

- 代码里若干关键脚本直接写死 `engine/vendor/bin/*.x`
- README 和历史日志中残留 `/home/ai/...`、旧 Linux 工作目录

本轮已新增统一平台解析模块，逐步把 native 路径改为通过 `engine/core/platform_utils.py` 和 `engine/core/runner.py` 获取。

## 当前平台相关风险

- 现有 `.x` 均为 Linux x86_64 ELF，不能直接跨平台复用
- 当前宿主机若不是 Linux x86_64，则这些原始 `.x` 无法直接运行
- 源码仓库当前没有统一 `bin/<platform>/` 发布结构
- ARM64 构建需要 Docker 多架构支持或 ARM64 Linux 环境
- 仓库解压后自带大量未提交改动，尤其 vendor 源码目录处于脏状态，本轮不回退这些内容
