# P6 CALMET A→G 顺序执行与验收台账

日期：2026-10-11。适用范围：合肥 2022-01-01 00:00 UTC 至 2022-01-06 23:00 UTC **144 个时间标签/143 个实际一小时时段**；5×5 km、50×50、100 m、9 层、UTM50N/WGS84。原版 CALMET 6.5.0 Level 150223。用户指令：**严格 P6A → P6B → P6C → P6D → P6E → P6F → P6G，先验收当前完整阶段，再进入下一阶段。**

**截至本轮原版独立验收（2026-10-11）最新有效门槛：P6A PASS；P6B PASS；P6C PASS_SCOPED_HEFEI_2022_OBSERVATIONS；P6D IN_PROGRESS / SOURCE_KERNELS_AND_SELECTED_DIAGNO_W_SEGMENT_PASS / FULL_2022_UVW_NOT_PASS；P6E/P6F/P6G 未通过整阶段验收；P6_CALPUFF_READY=FALSE。** 前文分期记录中标注的 P6C PARTIAL/HOLD 均为此前阶段的历史状态，不再代表最新结论。降水输入另行保持不合格，不能据此宣称湿沉降可用。详见 [P6C RDSN/CGAMMA 原版运行时最终审计](P6C_RDSN_CGAMMA_2022_AUDIT.md) 和 [CI #38113874913](https://github.com/RRain001/CTEAS/actions/runs/38113874913)。



### 2026-10-11 P6D 原版数值验收：内核与选定连续计算片段（最新）

独立原版 Fortran 数值 CI：[SLOPE](https://github.com/RRain001/CTEAS/actions/runs/38113822845)、[DIVCEL](https://github.com/RRain001/CTEAS/actions/runs/38115350648)、[MINIM/WINDBC](https://github.com/RRain001/CTEAS/actions/runs/38115632414)、[SMOOTH](https://github.com/RRain001/CTEAS/actions/runs/38115873780)、[DIAGNO 最终风场片段](https://github.com/RRain001/CTEAS/actions/runs/38116193083)。固定原版 CALMET 6.5.0 的源 blob 437f43bb7f0e51a1919e56664663e1a046986ed0，原版源文件未被修改。独立源码提取、编译和验证均通过；已核对非零风场、原版边界、分层记录顺序以及预设容差。

DIAGNO L6119–6148 的选定片段完整通过 SMOOTH→全层 DIVCEL→由下往上 W 积分→WINDBC 的原版原位程序顺序对照：18/18 CTest、450,000 个 U/V/DIV/W 数值和 7,200 个边界分量无超限项；最大 W 差异约 5.1e-11 m/s。MINIM 在选定控制 IKINE=0 / IOBR=0 下不会实际执行，仅保留为独立原版分支验证。

严格门槛：这些人工输入/源码片段 PASS **不能**替代原版真实 2022 气象驱动下的 WIND1/SIMILT/INTER2、FRADJ/SLOPE 组合以及 1h/6h/143h 最终 U/V/W 验证。P6D_FULL=NOT_PASS；P6_CALPUFF_READY=FALSE；湿降水驱动 NPSTA=0、IPCODE 143/143 缺测单独保持不合格。
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
| **P6C** 几何、观测预处理 | **PASS_SCOPED_HEFEI_2022_OBSERVATIONS** | 既有 TERSET/DIAGI、GEO 20,000、SURF 1,144、UP 6,280、VERTAV 234、PREPDI 2,574、FACET 130；[COMP+RDSN 143h 真实运行时](https://github.com/RRain001/CTEAS/actions/runs/38113500059) AA/BB 286 个时次、RDSN 1,144 个输入值、858 时间字段、0 差异；[CGAMMA 143h 原版实时调用](https://github.com/RRain001/CTEAS/actions/runs/38113874913) 86/51/6 三分支 PASS，最大误差 5.0E-11 K/m；原版 MET.DAT 插桩前后 SHA 相同 | 本阶段**仅**验收固定 2022 一地面虚拟站、一探空站、GEO/SURF/UP 与 143h 观测输入的选定配置；其他格式、多站与缺测回填分支未纳入支持范围。雨量另列不合格，不妨碍无雨诊断风场开发 |
| **P6D** 三维诊断风场 | **IN_PROGRESS / VERIFIED_KERNELS_AND_SELECTED_W_SEGMENT / FULL_NOT_PASS** | 原版 FRADJ（既有）；[SLOPE PR #9](https://github.com/RRain001/CTEAS/pull/9) 270,000 U/V；[DIVCEL PR #12](https://github.com/RRain001/CTEAS/pull/12) 90,000 散度+36 峰值；[MINIM/WINDBC PR #13](https://github.com/RRain001/CTEAS/pull/13) 270,000 U/V/DIV+7,200 边界；[SMOOTH PR #14](https://github.com/RRain001/CTEAS/pull/14) 180,000 U/V；[DIAGNO W 片段 PR #15](https://github.com/RRain001/CTEAS/pull/15) 450,000 U/V/DIV/W+7,200 边界；全部合并到 P6 开发分支，仅证明指定原版源码片段的人工场景数值等价 | 尚缺真实 2022 观测驱动 WIND1/SIMILT/INTER2、FRADJ/SLOPE 和 DIAGNO 全路径集成，以及原版 1h、6h、143h 逐层 U/V/W、掩码和散度对照；不得宣称 P6D 完成 |
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

### P6C 正式选定配置验收结论（覆盖前面的分期 PARTIAL 状态）

独立证据：[原版真实 143h RDSN/COMP PASS](https://github.com/RRain001/CTEAS/actions/runs/38113500059)；[原版真实 143h CGAMMA 与完整 P6B 回归 PASS](https://github.com/RRain001/CTEAS/actions/runs/38113874913)；[专项最终报告](P6C_RDSN_CGAMMA_2022_AUDIT.md)。通过了 1h→6h→143h 地形、地面观测、探空原始字段与垂直/时间风温插值源对照；`COMP` 物理 AA/BB 286 个时次、`RDSN` 1,144 个原版实际地面气象值、858 个时间关联值及 143 个 `CGAMMA` 直减率均为 **0 超限项**。温度直减率三路径样本为普通插值 86、当时次干绝热 51、先前对流记忆修正 6。插桩前后原版 `MET.DAT` SHA256 严格相等。

**决策：`P6C_SELECTED_CASE=PASS_SCOPED_HEFEI_2022_OBSERVATIONS`，P6D 的完整三维诊断风场集成可以开始，但绝不把已经 PASS 的孤立 `FRADJ` 误称为 P6D 完成。** P6E–P6G 仍未验收，`P6_CALPUFF_READY=FALSE`。湿驱动方面，真实 SURF `IPCODE=9999` 143/143、`NPSTA=0`：`P6_WET_FORCING_VALIDATED=FALSE`，`P7_WET_DEPOSITION_VALIDATED=FALSE`，与 P6C 选定配置通过彼此独立。
