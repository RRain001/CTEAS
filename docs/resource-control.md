# CTEAS 资源管理

部署主机 lhy-desktop；本次仅改变旧 Runner 1–3 的独立资源 drop-in、Gate 调度/并行度、测试和文档。数值代码、原版 Fortran、冻结 SHA、科学阈值、Runner 4–16、网络配置及 Actions 开关均不变。

## 预算与入口

- 新 Runner 4–16：既有 42 CPU 时间单位 / 72 GiB，不重新部署。
- 原 Runner 1–3：各 CPUQuota=400%，MemoryHigh=7G，MemoryMax=8G，MemorySwapMax=0，TasksMax=512，OOMPolicy=kill，KillMode=control-group。
- Gate 池：root-owned `cteas-gate.slice`，6 CPU、MemoryHigh=14G、MemoryMax=16G、无 swap；位于 `/cteas.slice/cteas-gate.slice`，不限制 lhy 的 SSH/user.slice。
- 所有预算合计 60 CPU 时间单位、112 GiB MemoryMax；CPU 时间单位不等于预留物理核心。剩余约 13 GiB 供系统及其他服务使用，这是预算余量，不是整台主机的 OOM 保证。

用户级 systemd 实测没有有效 cpu.max，因此不能采用它作为资源边界。root-owned `cteas-gate-control` 固定启动 User=lhy 的 transient service，资源参数仅接受 2/4/6 CPU、2–15 GiB；不接受任意 root 命令/属性。不新增常驻调度 daemon，不修改 sudoers，现有 lhy 管理账户通过 sudo -n 调用受控脚本。

```bash
cd /home/lhy/gate/driver
scripts/run_local_gate_limited.sh --track p6e
scripts/run_local_gate_limited.sh --track p7 --cpus 4 --memory-gib 10 --detach
CTEAS_BUILD_JOBS=2 scripts/run_local_gate_limited.sh --track p6d
```

默认一个任务请求 6 CPU / 15 GiB，因此默认串行。保留 1 GiB 给排队进程、Python 和调度管理。最多 32 个待运行服务。显式 2 CPU/4 GiB 与 4 CPU/10 GiB 可并行，总 CPU≤6、任务内存上限总和≤15 GiB；实际父 slice 仍强制 16 GiB 上限。同 track 永不同时运行。已排队任务按创建时间 FIFO，较大的回归任务不会被新提交的小任务无限绕过；可能存在头部阻塞，这是简单可靠策略的取舍。

request/state/evidence/log 位于 `/home/lhy/gate-managed/requests/<UUID>/`。正常运行保留已有 `/home/lhy/gate` 数据根，因为科学测试使用 driver 中的只读冻结输入；原 Gate 自带 PID+UUID 的每次独立 worktree 与 SHA+run-id 输出目录。包装器始终指定独立 evidence.json，默认汇总报告不再互相覆盖。显式 --report 会独占创建目标文件，不覆盖既有报告。source SHA 在提交时解析为完整 commit；移动 ref 在短 Git 锁内 fetch 后固定，执行过程中再次 fetch 不会改变请求 SHA。

SSH 断开只会失去客户端等待；PID 1 管理的服务及其子进程继续执行，结果持久写盘。`--detach` 立即返回 UUID。正常/失败退出均释放准入；PID 起始时间+boot ID 校验可识别异常终止/重启的陈旧状态，下次调度或客户端轮询将其标为失败，不伪造科学 PASS。主机重启不会自动重跑瞬态 Gate，需重新提交；Runner 服务仍开机自启。

查询与取消自己的任务：

```bash
cat /home/lhy/gate-managed/requests/UUID/state.json
sudo systemctl status cteas-gate-UUID.service
# 只取消自己明确选择的任务；整个子进程树会被终止。
sudo systemctl stop cteas-gate-UUID.service
```

## 动态并行度

优先使用 CTEAS_BUILD_JOBS 请求，但它会被资源上限钳制。读取当前 cgroups v2 挂载及 cpu.max，沿祖先取最小 quota/period，向下取整且至少 1；与 sched_getaffinity 和 cpuset.cpus.effective 的交集上限比较。cpu.max=max 时使用 affinity，默认安全上限为 8，绝不只用 os.cpu_count() 返回的 112。无法取得 affinity 时默认 4。进一步用有效 memory.high/memory.max，扣除 512 MiB 并按每编译进程 768 MiB 估算降低并行度；这是调度保守估算，不是对所有编译器内存的证明。

所有 5 个 track 的 cmake --parallel 24 已替换为动态 jobs；同时设置 CTEAS_BUILD_JOBS 与 CMAKE_BUILD_PARALLEL_LEVEL。invalid/非正请求失败，不无声采用任意值。受控 Gate 缺少实际 CPU 或内存 cgroup 时拒绝运行；不会静默退化。资源信息、cgroup metrics 与每步完整日志均加入证据，PASS/FAIL/BLOCKED、CTest 判定、案例 SHA 和对照程序不变。

直接执行改造后的 local_gate.py（--list 除外）会转交受控提交入口。未修改/历史副本、恶意手工跳过包装器、其他 lhy sudo 管理命令不在这条边界内；不能宣称限制了整个用户的任意进程。已修改的 driver 应在独立部署分支运行，不直接改 main。

## 安装、逐个应用与回滚

```bash
cd /home/lhy/cteas-resource-control
bash scripts/install_gate_control.sh
python3 scripts/legacy_runner_resources.py apply --runner 1
# 检查后再依次处理 2、3；不会修改或重新注册 4–16。
```

旧 Runner 基础 service/配置及旧资源 drop-in 原文保存在 `/home/lhy/cteas-resource-audit/*-before.json`，权限 0600。apply/rollback 只操作 cteas-resources.conf，校验 network drop-in SHA 不变；busy 时拒绝，并在重启前复查。不要将这些服务器备份或 gh 凭据提交到仓库。

```bash
bash /home/lhy/cteas-resource-control/scripts/rollback_resources.sh
```

一键回滚拒绝在 Gate active 时运行，也不会强制中断 Runner busy Job。仅恢复旧资源文件和 Gate slice 旧配置；网络、注册、数据、证据均保留。回滚后包装器保持 fail-closed，资源池未装回前不执行无限制 Gate。恢复旧 Gate 源码版本需在 driver 工作区干净、没有任务时显式切回记录的基线 commit；这是另外的代码回滚，不自动覆盖用户工作。

## 验证边界

既有服务的前次重启已重置 cgroup 历史峰值，空闲约 65 MiB 不能代表过去科学作业的峰值。报告使用本次实际测量，不把历史 PASS 时间与不同 SHA/driver 版本混为严格性能基准。测试从原分支固定科学 SHA，正常验证比较 CTest 数量、失败/BLOCKED、冻结字节/数值对照和真实 OOM events。CPU throttled_usec 是累计调度统计，并非全部作业墙钟时间损失。

其他 workflow 或科学目录内仍存在的固定并行参数本次只审计，不改物理/参考程序。原 Runner 的硬配额依然生效，但未经迁移的显式 --parallel 24 会覆盖环境变量的建议并行度，可能降低效率。计时敏感测试应另行测量；不可因此放宽科学阈值。
