# P6 CALMET A→G 顺序执行与验收台账

日期：2026-10-11。适用范围：合肥 2022-01-01 00:00 UTC 至 2022-01-06 23:00 UTC **144 个时间标签/143 个实际一小时时段**；5×5 km、50×50、100 m、9 层、UTM50N/WGS84。原版 CALMET 6.5.0 Level 150223。用户指令：**严格 P6A → P6B → P6C → P6D → P6E → P6F → P6G，先验收当前完整阶段，再进入下一阶段。**

## 0. 执行纪律

1. P6A 与 P6B 已有独立 PASS 证据，无需重复设计；每次 CI 应维护不回退。实际观测数据选取与气象质量合格不能仅由原版 CALMET 正常终止来推出。
2. P6C 必须达到 **COMPLETE** 才允许开始 **P6D 的整阶段集成**。之前已经单独开发、并通过原版 Fortran 对照的 `FRADJ` 是 **预先完成的独立子程序**，保留代码和证据，但绝不据此将 P6D 标为 PASS。原则上不再新增 P6D 完整流水线，直到 P6C 验收完成。
3. P6E、P6F、P6G 各自保留独立数值门槛和 PR 审查证据；**不把模拟成功、记录字段对齐或近似数值相符互相替代**。
4. 原版 `engine/vendor/src/**`、P1/P2/P3/P4/P5 各自分支及公共基础 `60b7b1d` 不允许被 P6 写入。P6 原版参考链对 P3/P4/P5 使用冻结 SHA。
5. 当前主线以 `NOOBS=0`, `IWFCOD=1`, `IPROG=0`, `IRTYPE=1`, `IFRADJ=1`, `ISLOPE=1`, `IEXTRP=-4` 的真实 2022 合肥案例为目标。未经真实控制确认的可选历史参数均不能偷偷启用。原版 `NPSTA=0` 允许风和微气象参考跑通，**但不具备后续湿沉降的有效雨量驱动**。P6 与 P7 的降水/WET 验证分别单列。

## 1. 阶段台账

| 阶段 | 当前正式阶段状态 | 已固化的证据 | 下一次完整验收必须满足 |
|---|---|---|---|
| **P6A** 程序架构/输入与时间契约 | **PASS_SOFTWARE_CONTRACTS** | C++20 `-Werror` CTest，50×50/100m/9 层、显式观测有效性、时区和输入源门槛；旧样例 `CALMET.INP` 拒绝 | 保持回归，接口不得静默变化 |
| **P6B** 原版实算基准 | **PASS_ORIGINAL_REFERENCE_DRY_MICROMET** | [源 SHA 锁定独立原版 CI](https://github.com/RRain001/CTEAS/actions/runs/38107162904)；`MET.DAT` 64,762,556 bytes、SHA256 `968b2dd24f0eb4ad428709b52a154d9022e8c7e37a4be3fb9498e3d7e756d04e`；7,562 顺序记录、143 时段×45 字段 | 来源冻结、输出格式解码持续 PASS；降水基准另列，不要求用虚构雨量冒充 |
| **P6C** 几何、观测预处理 | **IN_PROGRESS / NOT_COMPLETE** | `TERSET` 60,000 字段 PASS；`DIAGI` 378 原版片段字段 PASS；2022 `GEO.DAT` 20,000 数值 PASS；2022 `SURF.DAT` 1,144 气象值 +429 站时值 PASS；2022 `UP.DAT` 13探空/785层、6,280 原版数值 PASS；**源码原样 `VERTAV` 13×9×U/V=234 PASS；原版 `PREPDI` 143h×9×U/V=2,574 PASS；源码原样 `FACET+INTP` 13×10=130 PASS**。2022 降水代码 143/143 缺测。 | 仍欠完整原版 `COMP` AA/BB 缓冲状态、逐时 `PREPDI` 站点风场与温度/缺测状态集成、完整 `READGE/RDSN/RDUPN` 调用状态，所有真实数据的 1h→6h→143h 原版观测场逐值对比。**未验收前不得标 P6C COMPLETE**。 |
| **P6D** 三维诊断风场 | **NOT_STARTED_INTEGRATION / FRADJ_ISOLATED_PASS** | 旧已实现 `TERSET+FRADJ` 独立源码数值对照：7 案例 315,000 U/V 字段 PASS，稳定弱风山峰有 5,657 个原版风分量真实调整 | P6C 完成后：`SLOPE`、`WIND1`、`SIMILT`、`DIAGNO`、`TOPOF2`、平滑/客观分析、`DIVCEL/MINIM`，原版首小时→6 h→143 时段逐层 `U/V/W`、散度与正确值掩码 PASS |
| **P6E** 地表与边界层 | **NOT_STARTED** | P6B 原版包含微气象字段格式，不代表 C++ 物理 PASS | `SOLAR`、`HEATFX`、`ELUSTR`、`MIXHT`、`TEMP3D`、`SURFVAR`、云量/RH/稳定度/混合层高度/地表参数全字段独立对照；白天、夜间、低风与稳/不稳定 |
| **P6F** `CALMET.DAT` 写出 | **NOT_STARTED_CPP_WRITER** | 原版二进制 v2.1 记录框架、字段名、时间标签、顺序和维度的独立读取解码 PASS | C++ 完整合法输出 v2.1，原版 P7 读取，物理值正确；不能仅以文件可读取作为科学数值通过 |
| **P6G** 全链条科研验收 | **NOT_STARTED** | 已有原版固定对照；需等待 P6C→F | 同源同控制的 143 个完整时段、逐网格逐高度逐场对照及独立审计；按预注册误差门槛和质量分母给出 PASS/FAIL；站点代表性/雨量/扩散结果分别报告 |

## 2. 新增 P6C 站点几何子模块：原版真实源码依据

- 原版 `engine/vendor/src/CALMET_v6.5.0_L150223/calmet.for`: `DIAGI` **L5097–5117** 为探空站参考网格索引 `IIUP,JJUP`，按 `XUSTA/DGRID+1.5` 赋给 Fortran INTEGER，再收敛到 1–NX/NY；**L5123–5145** 为地面/探空站风场内部坐标 `IST,JST,UTMXST,UTMYST`；**L5163–5182** 为地面站粗糙度格网边界钳制及 `ZLOGSTA` 对数廓线高度系数。`READCF` **L29766** 的 `ZMID=0.5(ZFACE(k+1)+ZFACE(k))` 提供第一层中心高度（10 m）。
- 这里存在**两种不同的映射规则**：探空站参考格点用“+1.5 后取整并钳制”；地面/探空风站内部格点用“+1 后取整”，**原版没有对 IST/JST 全局钳制**，仅为选择 `z0` 网格执行边界钳制。研究域外的站点仍有物理位置，不能凭空放进 5×5 km。
- 现代源码：`native_cpp/modules/p6_calmet/include/cteas/p6/station_geometry.hpp`、CLI `cteas_p6_stations`、`p6_station_geometry_tests.cpp`。
- 独立验证 `compare_original_diagi_station.py` **从原版 calmet.for 提取不改变的实际 Fortran 行**，编译为原版源码片段驱动，与 C++20 跑相同人工场景。三组不同粗糙度、内站/外站/半格点/边界/不同风速计高度共 **42 条站记录，378 个对照字段**；最大浮点绝对差 `4.791997909592283×10^-10`，无不合格项。**这证明列出的 DIAGI 片段数值一致，不代表完整 DIAGI 真实气象执行一致**。
- 最新证据：[P6C DIAGI station source code parity CI PASS](https://github.com/RRain001/CTEAS/actions/runs/38107845203)（包含四项 CTest 的通过和 Fortran 片段编译/比较证据）。
- 本阶段实现会对非法坐标、非有限数、粗糙度零或风速计高度接近粗糙度造成零对数分母采取 fail-closed。原版不一定主动报错；此安全提升不宣称完全相同的错误行为。

## 3. P6C 下一验收工作包（顺序）

已通过 `C-READGE` 的 8 字段选定格式原生 Fortran 逐值对照；`C-MET-READ` 中 **2022 SURF.DAT 及 2022 UP.DAT v2.1 对应原版 RDSN/RDUPN 的选定格式和逐层 U/V 转换均已 PASS**；但 **原版 `PREPDI` 的选定 143h 时间插值数值已 PASS，但完整原版 `COMP` AA/BB 逐小时状态机仍未通过**；接下来依次推进 `C-HEIGHT-TIME`（`ZFACE`、`ZMID`、站高、探空层与时区端点一致性）再做 `C-INTEGRATION`（同一份原版输入的观测场比较和缺测统计）。**只有四项齐备且经独立 Fortran 片段或完整执行验证，才由 P6C → P6D。**

**最终门槛声明：`P6A=PASS`, `P6B=PASS`, `P6C=PARTIAL`, `P6D=INTEGRATION_HOLD`, `P6E=HOLD`, `P6F=HOLD`, `P6G=HOLD`, `P6_CALPUFF_READY=FALSE`。**

### P6C 新增 2022 真实读取证据（2026-10-11）

详见 [P6C_READGE_SURFACE_2022_AUDIT.md](P6C_READGE_SURFACE_2022_AUDIT.md)：[READGE 20,000 数值 PASS](https://github.com/RRain001/CTEAS/actions/runs/38109436038)；[SURF.DAT 原生 Fortran 1,144 数值、429 站/时值 PASS，且原版 P6 重新运行 PASS](https://github.com/RRain001/CTEAS/actions/runs/38109730617)。2022 SURF 缺测统计：风矢量 0/143、云量 0/143、**降水代码 143/143 缺测**。格式数值 PASS 不表示天气源数据正确，也不表示降水有效。C-MET-UP 选定格式和 U/V 转换已完成；真实 2022 AA/BB 时次、垂直插值及完整原版观测场比较仍是 P6C 的硬性阻塞。

### 2022 探空 RDHDU→RDUPN 路径纠正及实测对照

[探空读取独立审计](P6C_UPDAT_RDUPN_2022_AUDIT.md)；[真实 2022 Fortran/C++ 13 sounding / 785 levels / 6,280 numeric fields PASS](https://github.com/RRain001/CTEAS/actions/runs/38110557735)。原版 `calmet.for` 的 `COMP` 分派表明 v2.1 使用 **`RDUPN`**，而旧版 `RDUP` 不适用此案例。实际标记：`UPDAT_SELECTED_FORMAT_PASS`，`UPDAT_ORIGINAL_CONVERSION_PASS`，`UPDAT_AA_BB_TIME_INTERPOLATION_NOT_READY`；仍然不允许跳至 P6D 整阶段实现。

### P6C 实测 VERTAV / PREPDI / FACET 温度层面最新结果

[原始 CALMET Fortran 2022 单元对照 CI](https://github.com/RRain001/CTEAS/actions/runs/38111692047) 对 `VERTAV` 的 234 个层平均 U/V（原版完整子程序提取），`PREPDI` 的 2,574 个分层逐时 U/V（原版指定时间插值源码片段与独立时次索引），`FACET+INTP` 的 130 个模型层界面温度（原版两个完整子程序提取）均 PASS。详见 [P6C_VERTAV_PREPDI_FACET_2022_AUDIT.md](P6C_VERTAV_PREPDI_FACET_2022_AUDIT.md)。注意：这并不证明 `COMP` 的全部状态机，也不能取代微气象模块和 P6D 诊断风场。正式标记维持 `P6C_PARTIAL`、`P6D–G_HOLD`。

### P6C 2026-10-11 真实 COMP 双缓冲 + 三源联合输入进展（保持整阶段 PARTIAL）

本次新增 `comp_buffered_timeline.hpp`、`comp_joint_main.cpp`、`check_joint_observation_2022.py`、`original_comp_runtime_trace.py`，以及软件测试与原版独立验证。详细审计 [P6C_COMP_JOINT_2022_AUDIT.md](P6C_COMP_JOINT_2022_AUDIT.md)。

- [原版 Fortran 143 时段 COMP 双缓冲运行状态 PASS](https://github.com/RRain001/CTEAS/actions/runs/38112842109)：286 个物理 AA/BB 探空时次，429 个时间/秒相关检查，11 次 `JUSTA/JORDER` 切换与 C++ 一致，0 mismatches；临时插桩前后原版 MET.DAT SHA256 相同。没有修改 vendor Fortran 原文件。
- [真实三源联合场候选链与原版数值核验 PASS](https://github.com/RRain001/CTEAS/actions/runs/38112477819)：GEO 50×50、SURF 143 小时、UP 13 探空；1/6/143 时段，143×9×2=2,574 U/V 与 5,148 个 AA/BB 层面温度关联值通过，地面站/时间/云量/温度/风有效性掩码一致。最大 U/V 误差约 `5e-10 m/s`。
- **原版 `MAXSEP=14 h`** 是 `calmet.for` 的 `data maxsep/14/` 固定常量（L3654），不是 CALMET.INP 的可配置 24h。已纠正原生默认限制，测试 15 h 缺测 fail-closed；频率严格检查原版 `abs(IUPSEC)<NSECDT && IUPSEC != 0`。
- 仍欠原版 `RDSN/CGAMMA` 联动及完整观测掩码/输入场执行状态验收。不能因 COMP 状态 PASS 就认定 `P6C_COMPLETE=TRUE`，也不能开始 P6D 整阶段。
- P5 SURF 的降水 `IPCODE=9999` **143/143 缺测**；原版 `NPSTA=0`。`P6_WET_FORCING_VALIDATED=FALSE`、`P7_WET_DEPOSITION_VALIDATED=FALSE` 独立保持。
