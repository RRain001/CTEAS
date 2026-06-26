# CTGPROC 自动配置与执行说明

本目录提供一个脚本，用于基于 `grid.cmn` 自动更新 `ctgproc.inp` 的必要参数，并执行 CTGPROC 生成 `lu.dat`。

## 脚本

- [auto_select_landuse_to_ctgproc.py](/home/ai/code/calpuff/tutorial/data/ctgproc/auto_select_landuse_to_ctgproc.py)

## 固定土地利用数据

当前默认直接使用单一土地利用文件（亚洲/欧洲覆盖）：

- `/home/ai/calpuff_terrain_and_landuse_data/eurasia_asia_landuse/eausgs2_0la.img`

不再做目录扫描和自动挑选多文件。

## 会自动同步/修改的内容

脚本会从 `/home/ai/code/calpuff/tutorial/data/grid/grid.cmn` 同步以下参数到 `ctgproc.inp`：

- `PMAP`
- `IUTMZN`
- `UTMHEM`
- `DATUM`
- `XREFKM`
- `YREFKM`
- `NX`
- `NY`
- `DGRIDKM`

同时会更新：

- `NDBF = 1`
- `Subgroup (0b)` 中的 `1 !GLAZAS = ... !    !END!`

## 与副本文件对比后的必要改动结论

相对于 `ctgproc - 副本.inp`，当前 `ctgproc.inp` 仅有以下必要差异：

- `GLAZAS` 路径改为本机可用 Linux 路径
- `XREFKM: 433.385 -> 493.384`
- `YREFKM: 3453.769 -> 3513.683`
- `NX: 160 -> 40`
- `NY: 160 -> 40`

其余关键控制项保持不变（如 `PMAP/IUTMZN/UTMHEM/DATUM/DGRIDKM/LCFILES/LFINAL/LPREV/MESH*`）。

## 运行方式

在任意目录执行：

```bash
python3 /home/ai/code/calpuff/tutorial/data/ctgproc/auto_select_landuse_to_ctgproc.py
```

执行后会在 `/home/ai/code/calpuff/tutorial/data/ctgproc/` 生成或更新：

- `ctgproc.inp`
- `ctgproc.lst`
- `lu.dat`

## 常用参数

只预览修改，不写文件、不运行 CTGPROC：

```bash
python3 /home/ai/code/calpuff/tutorial/data/ctgproc/auto_select_landuse_to_ctgproc.py --dry-run
```

只更新 `ctgproc.inp`，不执行 `ctgproc.x`：

```bash
python3 /home/ai/code/calpuff/tutorial/data/ctgproc/auto_select_landuse_to_ctgproc.py --no-run-ctgproc
```

覆盖默认土地利用文件：

```bash
python3 /home/ai/code/calpuff/tutorial/data/ctgproc/auto_select_landuse_to_ctgproc.py \
  --landuse-file /path/to/your_landuse.img
```
