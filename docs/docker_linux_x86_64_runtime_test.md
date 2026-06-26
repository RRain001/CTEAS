# Docker Linux x86_64 Runtime Test

脚本：`scripts/test_linux_x86_64_runtime_in_docker.sh`

## 当前推荐策略

- 默认镜像：`debian:12`
- 默认不启用代理：`USE_DOCKER_PROXY=0`
- 仅在必要时启用：

```bash
USE_DOCKER_PROXY=1 PROXY_PORT=7897 ./scripts/test_linux_x86_64_runtime_in_docker.sh
```

- Docker runtime 测试不安装 `PySide6`
- Docker runtime 测试使用最小依赖文件：
  - `requirements-docker-runtime.txt`
  - 当前内容：`pyyaml==6.0.3`、`numpy==2.4.4`、`pandas==3.0.2`

## 默认测试 case

- 默认 case 已改为：`smoke5d_docker_test`
- 测试前已复制：

```bash
cp -a project/cases/smoke5d project/cases/smoke5d_docker_test
```

- 测试后已确认当前工作 case 仍为：
  - `current_case.yaml -> smoke5d`
  - `project/current -> cases/smoke5d`

## 已执行命令

```bash
./scripts/test_linux_x86_64_runtime_in_docker.sh
```

容器参数：

- `--platform linux/amd64`
- 旧策略镜像：`python:3.12-slim-bookworm`

## 实际结果

主测试未能进入项目运行阶段，失败发生在容器环境准备阶段，属于外部环境/网络问题，不是仓库内科学计算逻辑问题。

### 第一次执行

失败点：

- Docker Hub 拉取镜像鉴权阶段

关键报错：

```text
docker: failed to authorize: failed to fetch oauth token:
Post "https://auth.docker.io/token": net/http: TLS handshake timeout
```

失败分类：

- Python 路径问题：否
- `.x` 程序运行错误：否
- MET/PUFF 输入配置错误：否
- 缺 Linux 动态库：否
- 缺输入文件：否
- 权限问题：否
- 容器外部网络 / 镜像拉取问题：是

### 第二次执行

失败点：

- Docker Hub manifest 请求阶段

关键报错：

```text
docker: Error response from daemon: failed to resolve reference
"docker.io/library/python:3.12-slim-bookworm":
failed to do request: Head "https://registry-1.docker.io/v2/library/python/manifests/3.12-slim-bookworm":
net/http: TLS handshake timeout
```

失败分类：

- 容器外部网络 / 镜像拉取问题：是

### 本地镜像回退探测

为了确认不是仓库代码问题，额外探测了本地已有镜像 `debian:12`：

1. `docker images` 显示本地已有：
   - `debian:12`
   - `alpine:latest`
2. 在 `debian:12` 容器内做 `apt-get update` 最小探测，报错：

```text
E: Failed to fetch http://deb.debian.org/debian/dists/bookworm-updates/main/binary-amd64/Packages  502  Bad Gateway
```

这进一步说明 Docker 容器内的外部网络访问本身不稳定，阻断了后续 Python/依赖准备，因此无法进入：

- `prepare_met`
- `prepare_puff`
- `run_read62`
- `run_met`
- `run_puff`

## 当前结论

- 已按计划实际执行 Docker linux/amd64 runtime test。
- 当前阻塞点是容器网络层：
  - Docker Hub TLS 握手超时
  - Debian 源 `bookworm-updates` 返回 `502 Bad Gateway`
- 在未修复容器网络前，无法把这次失败归因到项目代码、native `.x`、输入文件或 MET/PUFF 配置。

## 新增诊断与代理支持

本轮新增：

- `scripts/docker_network_diagnose.sh`
- `docs/docker_setup_for_macos_clash.md`

并更新了：

- `scripts/test_linux_x86_64_runtime_in_docker.sh`

当前 Docker runtime 脚本支持以下环境变量：

```bash
USE_DOCKER_PROXY=1
PROXY_HOST=host.docker.internal
PROXY_PORT=7897
```

代理启用示例：

```bash
USE_DOCKER_PROXY=1 PROXY_PORT=7897 ./scripts/test_linux_x86_64_runtime_in_docker.sh
```

诊断示例：

```bash
PROXY_PORT=7897 bash scripts/docker_network_diagnose.sh
```

旧策略下，如果拉取 `python:3.12-slim-bookworm` 失败，脚本会直接提示：

```text
Docker image pull failed. Check Docker Desktop proxy settings or run with USE_DOCKER_PROXY=1.
```

如果容器内 `apt-get update` 失败，当前脚本会直接提示：

```text
Container apt network failed. Check Clash Allow LAN and Docker proxy settings.
```

## 新诊断结果

已实际运行：

```bash
PROXY_PORT=7897 ./scripts/docker_network_diagnose.sh
```

得到的结论是：

- Docker 本身可用。
- `linux/arm64` 容器可启动，`uname -m` 返回 `aarch64`。
- `linux/amd64` 容器可启动，`uname -m` 返回 `x86_64`。
- 容器无代理时，`debian:12` 的 `apt-get update` 本次成功。
- 容器可以连到宿主机代理端口：`host.docker.internal:7897`。
- 容器启用代理后，`debian-security` 通过代理返回 `502 Bad Gateway [IP: 192.168.65.254 7897]`。
- `python:3.12-slim-bookworm` 的 Docker Hub 拉取在两种情况下都不稳定：
  - 无代理：`TLS handshake timeout`
  - 代理相关重试：`failed to fetch oauth token: EOF` / `TLS handshake timeout`

因此当前更精确的判断是：

- Debian 主源并非完全不可达。
- 代理链路并非完全不可达。
- 但 Docker Hub 鉴权/manifest 拉取仍不稳定。
- Clash 代理下的 Debian security 源当前也不稳定。

## 当前脚本调整

本轮已把 runtime 脚本改为：

- 默认使用本地已有的 `debian:12`
- 容器内通过 `apt` 安装 `python3/python3-venv/python3-pip`
- `pip` 只安装 `requirements-docker-runtime.txt`
- `apt-get update`
- `apt-get install`
- `pip install`
  都带简单 `retry` 重试

这条路线的目标是：

- 避开默认拉取 `python:3.12-slim-bookworm`
- 降低 GUI 依赖带来的下载体积和失败概率
- 优先验证后端 runtime

## 更新后的 runtime 脚本验证

旧脚本验证时，已实际看到：

```bash
./scripts/test_linux_x86_64_runtime_in_docker.sh
```

结果：

- 脚本按预期打印了：
  - `Docker platform: linux/amd64`
  - `Test case: smoke5d_docker_test`
  - `Proxy enabled: 0`
  - `Proxy host: host.docker.internal`
  - `Proxy port: 7897`
- 在镜像拉取失败后，脚本按预期输出了明确提示：

```text
Docker image pull failed. Check Docker Desktop proxy settings or run with USE_DOCKER_PROXY=1.
```

## 本轮执行说明

本轮已经完成：

- `requirements-docker-runtime.txt`
- `debian:12` 默认镜像切换
- no-proxy 默认策略
- runtime 脚本重试逻辑
- 文档更新

但在继续实跑更新后的 `debian:12` runtime 脚本时，当前会话的 Docker 提权执行额度被平台拦截，因此这轮没有拿到新的容器内 `apt/pip/prepare_met` 实跑结果。

因此当前最新的事实边界是：

- 新脚本逻辑已改完并通过 `bash -n`
- 旧网络诊断结果仍然有效
- 新的 `debian:12` runtime 全流程需要在下一次可用的 Docker 提权执行中继续验证

## 已完成但可独立验证的前置项

以下内容已在宿主机完成验证：

- `scripts/test_gui_does_not_switch_current_case.py` 通过，`MainWindow` 初始化不会切换 current case。
- `scripts/check_native_binaries.py --platform linux-x86_64` 通过。
- `smoke5d_docker_test` 已创建，未污染 `smoke5d` 当前工作 case。
