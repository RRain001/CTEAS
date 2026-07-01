# Run GUI

## 启动前准备

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -U pip
pip install -r requirements.txt
```

## 启动 GUI

```bash
python -m engine.ui.desktop.main
```

或：

```bash
./calpuff-cli desktopgui
```

## 启动 CLI 编排入口

```bash
./calpuff-cli
```

## native 路径解析

当前 native 程序从以下目录解析：

- `./bin/<platform>/`

相关模块：

- `./engine/core/platform_utils.py`
- `./engine/core/runner.py`
- `./engine/core/paths.py`
