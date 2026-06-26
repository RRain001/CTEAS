# Run GUI

## 启动前准备

```bash
cd /Users/rain/CTEAS/tutorial
python3 -m venv .venv
source .venv/bin/activate
pip install -U pip
pip install -r requirements.txt
```

## 启动 GUI

```bash
cd /Users/rain/CTEAS/tutorial
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

- `/Users/rain/CTEAS/tutorial/bin/<platform>/`

相关模块：

- `/Users/rain/CTEAS/tutorial/engine/core/platform_utils.py`
- `/Users/rain/CTEAS/tutorial/engine/core/runner.py`
- `/Users/rain/CTEAS/tutorial/engine/core/paths.py`
