# CTEAS 七程序 C++ 独立重构总计划（Program 00）

状态：PROPOSED / BASELINE_AUDIT_NOT_RUN。日期：2026-10-10。
基线：CTEAS/main @ 69180557b97b99a2ffbf7f7a786909e570a06d6b。
本文件仅建立研究与工程工作范围，不表示七程序已移植、Runner 已运行或科学验证通过。

## 1. North star

在 RRain001/CTEAS 内保留既有 Python GUI / orchestration / Fortran 可运行链作为不变的参考系统，在隔离的 native_cpp/ 目录按源码行为独立实现现代 C++20 的 TERREL、CTGPROC、MAKEGEO、READ62、SMERGE、CALMET、CALPUFF。以固定版本原版 Fortran 的中间输出为数值参考，以理论/文献为物理说明，以真实 Hefei 2022 案例作为贯通验收。不得将“可编译”、“程序输出文件存在”、“限定子集 PASS”混同于完整算法一致或科学有效性。

目标按三个等级分别验收：
- A 软件/格式一致：配置、网格、单位、时序、完整记录、缺测、输入输出及守恒。
- B 原版行为一致：相同输入、相同选项、相同参考版本下对中间场、最终结果逐字段、逐数值比较，并解释不能比较的分支。
- C 科学有效性：在真实案例下与独立观测、解析解、敏感性试验比较。A/B 均不能替代 C。

## 2. Source-of-truth and freeze

所有旧代码仅供 reference 使用，固定以下目录为目标版本（路径相对本仓库）：
- engine/vendor/src/TERREL_v7.0.0_L141010
- engine/vendor/src/CTGPROC_v7.0.0_L150211
- engine/vendor/src/MAKEGEO_V3.2_L110401
- engine/vendor/src/READ62_v5.661_L110225
- engine/vendor/src/SMERGE_v5.7.0_L121203
- engine/vendor/src/CALMET_v6.5.0_L150223
- engine/vendor/src/CALPUFF_v7.2.1_L150618

首先登记每个源码树 SHA、编译 flags、依赖文件、控制文件、Fortran 输出和所用输入数据哈希；不要将当前 legacy 可运行状态当作已验证的全功能真值。任何源代码恢复、原版参考补丁必须有独立、固定的 provenance 和数值对照。不得以新的 C++ 结果重新生成所谓独立“参考期望值”。

不得覆盖 vendor/src、bin/ 下原始可运行程序、当前工程配置、GUI 或历史 case 结果。不要机械复制 Fortran；独立说明方法、验证知识产权/再分发许可，第三方大数据、许可证不明内容和原版二进制不得加入新 C++ 路线的提交。

## 3. Separate implementation and validation

新实现建议目录（尚未创建）：
- native_cpp/CMakeLists.txt
- native_cpp/include/cteas/...（公用基础类型、geo、met、puff）
- native_cpp/src/common, geo, met_input, calmet, calpuff
- native_cpp/tests/{unit,integration,reference}
- tools/reference_capture/（仅验证，可调用只读 Fortran）
- docs/reconstruction_cpp/（方法、证据矩阵、ADR）

保持 C++ 数值计算与配置/I/O/GUI 分离。创建单独 CMake target，首次只依赖稳定、明确授权的输入数据与必要系统库。借鉴 CTEAS-Atmos 的数值契约与经验，但 CTEAS-Atmos 暂仅作只读设计/比较参考；不得在未核查接口与许可时复制模块或引入跨仓库运行依赖。

## 4. Baseline case and reference data

唯一首个真实验收案例：仓库 testcase.md 所述 Hefei 2022 烟囱案例。规范与尚未决策的问题见 HEFEI_2022_REFERENCE_CASE.md。冻结站点、原始 DEM、土地利用、地表/高空气象数据、数据时间覆盖、坐标投影/基准面、各程序控制文件、经验参数/默认值及 reference 输出目录。项目忽略的 project/cases/ 和原始地理数据不能被假定已经存在于 Git 或 Runner。

每套程序须具备同输入 Fortran reference capture，再做最小合成、实际数据和结果差异定位。审计具体参考格式时，不仅哈希全文件，还应解码关键字段作数量、值域、位置、单位、掩码与科学意义校验；日志/运行时间戳等非科学字节与真正科学数据分开比较。

## 5. Critical path

P0 参考平台与 case 冻结（先做；不得跳过）：实际核查 Self-hosted Runner 在线状态、权限标签和隔离目录，复核原版七个 executable/build，冻结 2022 案例输入、逐程序端到端 reference 中间产物、全部源码/输入/输出哈希和失败目录。若缺任一关键输入，标记 BLOCKED_BY_SOURCE，不能造“真实”补值。

P1 TERREL（可与 P2 并行研究）：DEM 选择与元数据、投影、网格覆盖、地形插值、海陆/缺测、边缘。输出地形中间字段并验证。

P2 CTGPROC：土地利用源、原始类别与归并、覆盖比例/主导类、投影采样、缺测及岸线。不可把未覆盖区域默认为水体。

P3 MAKEGEO（依赖 P1+P2）：合并地形、土地利用及地表物理属性，固定 land-use-to-parameter lookup、GEO 文件版本与场位置。逐字段对照原版。

P4 READ62：高空气象格式转换、时间与探空垂直层、缺测、单位与站点高度。冻结新旧结果及 header/records。

P5 SMERGE（与 P4 可并行实施）：地面气象、风速风向、云量、降水及 QA、时间采样、缺测、站点 ID。复用已存在的 ISD 数据处理决策，不为凑结果任意填值。

P6 CALMET（依赖 P3/P4/P5）：先实现源程序实际气象计算依赖顺序及热力层（HEATFX、ELUSTR、PGTSTB、MIXHT、WATER），再风场初始化、地形效应、blocking、slope flow、observation objective analysis、DIVCEL/MINIM 等与 CALMET.DAT 输出。必须以原版同分支中间量验收每一步，不得仅凭 CTEAS-Atmos 的 Stage38 pass 标题继承等价性。

P7 CALPUFF（依赖 P6 或先使用冻结原版 CALMET 输出验证）：先确定模型输入和烟团状态及源项定义，逐步实现烟团释放、抬升、平流、湍流扩散、沉降/衰变、受体累加、输出及活动量账本。HTO 物种处理需独立批准：不能将通用 inert tracer 的 PASS 自动标记为完整 HTO 干湿沉降和剂量链。

P8 完整科学闭环：同一个冻结的 2022 case 逐层替换 Fortran 引擎，先替换 P1→P3，再替换 P4/P5，随后 CALMET，最后 CALPUFF；分别保存纯 Fortran、混合流程、纯 C++ 三个身份清晰的输出树。复验多时间点、全域、站点、预算、敏感性、独立观测、跨进程确定性；产出差异清单与不支持选项矩阵。

依赖图：
~~~text
TERREL ----\
            MAKEGEO --\
CTGPROC ---/           \
                        CALMET ----> CALPUFF
READ62 ----------------/
SMERGE ----------------/
~~~

## 6. Module gating policy

每个 P 阶段需提供：source routine/line-range & frozen SHA -> mathematical/empirical description -> input/output/sentinel/time/space contract -> C++ implementation -> unit/analytic tests -> Fortran capture -> bounded numeric comparator -> real case coverage -> regression -> independent review。分别报告 IMPLEMENTED、PASS_SCOPED、UNVERIFIED、NOT_COMPARABLE、BLOCKED。失败保留原始 evidence，禁止放宽容差、减少分母、替换实测输入、修改 Fortran 输出或通过伪造 expected values 让检查绿灯。

先定义接受域和精度规范；Fortran REAL32 与 C++ double 允许明确标识的量化差异，不要求不必要的 bitwise 相等。科学预算和站点对比优于只验证输出文件存在。

## 7. GitHub and server policy

新的长线 branch = codex/cpp-fullchain-reference-reconstruction。main/现有 GUI/orchestration/vendor/src 为默认稳定基线。后续使用短期子分支和 draft PR，每个有明确单一程序/物理功能，合并前独立审计，绝不 force-push 原始历史。当前只建立文档，尚未配置 workflow、未验证 Runner 在线/labels。

自托管 Linux Runner 建议仅执行显式授权的 workflow_dispatch / 审核过的受信分支测试，使用单独非 root 用户、资源与时长上限、并发限制、临时工作目录和 artifact 体积/保留期控制；不要对不受信任 PR 提供有私人数据/Secrets 的 Runner，禁止工作流泄露站点原始材料。Runner 已安装不等于现有 GitHub Actions workflow 已存在或已跑通；逐项验证。

P0 必须先决定 Linux 数据和执行路径，验证 Runner 的 Linux x86_64 GCC/gfortran/CMake/Python 版本、资源余量及读取外部私有 case 的权限。首个 Runner 工作流应为轻量、手动触发、不编译全部七个大程序的 preflight。

## 8. Success definition and unresolved decisions

全七程序的最终声明须有完整 2022 案例、逐程序共同子集比较、证据封存、独立观测适用性、不支持选项矩阵、可重复独立 C++ build/run；不能以“部分案例 PASS”声称原版全选项等价。
目前开放决策：5 km 范围究竟是半径/半边长/方形总边长；时间标注的 UTC/当地时间和首尾时刻语义；坐标投影及地表/高度基准；2022 真实数据与原版已运行 reference 是否同一场景；原版运行配置中的 CALMET/CALPUFF physics flags；HTO 排放率从年活动量转换及 6 日释放规则；观察数据泄漏与独立性。上述应在 P0 核实，禁止默认猜测。

第一提交只批准计划/证据目录。P1 之前先完成 P0，不得在没有 frozen reference 的情况下写大量地形/土地利用替代程序。
