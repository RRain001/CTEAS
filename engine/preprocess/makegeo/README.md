# MAKEGEO 自动配置与执行说明

本目录提供脚本，用于自动同步 `makegeo.inp` 的必要参数，并执行 MAKEGEO 生成 `geo.dat`。

## 脚本

- [auto_config_makegeo.py](/home/ai/code/calpuff/tutorial/data/makegeo/auto_config_makegeo.py)

## 为什么需要自动配置

`makegeo.inp` 原始模板里存在旧项目路径与旧网格参数（如 `XREFKM/YREFKM/NX/NY`），如果不改会与当前 `CTGPROC/TERREL/grid.cmn` 不一致，导致输入文件找不到或网格不匹配。

## 自动修改项（必要项）

脚本会从 `/home/ai/code/calpuff/tutorial/data/grid/grid.cmn` 同步：

- `PMAP`
- `IUTMZN`
- `UTMHEM`
- `DATUM`
- `XREFKM`
- `YREFKM`
- `NX`
- `NY`
- `DGRIDKM`

脚本还会更新输入文件路径：

- `LUDAT` -> `/home/ai/code/calpuff/tutorial/data/ctgproc/lu.dat`
- `TERRDAT` -> `/home/ai/code/calpuff/tutorial/data/terrel/terrel.dat`

并保留备份文件：

- `makegeo.inp.bak.auto`

## 默认执行命令

脚本最后会执行：

```bash
/home/ai/code/calpuff/tutorial/MAKEGEO_V3.2_L110401/makegeo.x makegeo.inp
```

工作目录为 `makegeo.inp` 所在目录。

## 运行方式

```bash
python3 /home/ai/code/calpuff/tutorial/data/makegeo/auto_config_makegeo.py
```

## 常用参数

只预览，不写文件、不运行：

```bash
python3 /home/ai/code/calpuff/tutorial/data/makegeo/auto_config_makegeo.py --dry-run
```

只改 `makegeo.inp`，不执行 MAKEGEO：

```bash
python3 /home/ai/code/calpuff/tutorial/data/makegeo/auto_config_makegeo.py --no-run-makegeo
```

覆盖输入文件：

```bash
python3 /home/ai/code/calpuff/tutorial/data/makegeo/auto_config_makegeo.py \
  --ludat /path/to/lu.dat \
  --terrdat /path/to/terrel.dat
```

## 本次验证结果

已成功生成以下输出文件：

- `/home/ai/code/calpuff/tutorial/data/makegeo/geo.dat`
- `/home/ai/code/calpuff/tutorial/data/makegeo/makegeo.lst`
- `/home/ai/code/calpuff/tutorial/data/makegeo/qaluse.grd`
- `/home/ai/code/calpuff/tutorial/data/makegeo/qaterr.grd`

`makegeo.lst` 中未发现 `ERROR`，且网格信息为 `NX=40, NY=40`，与当前配置一致。
