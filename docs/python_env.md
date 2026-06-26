# Python Environment

## 依赖文件

- 已有：`/Users/rain/CTEAS/tutorial/requirements.txt`
- 未发现：
  - `environment.yml`
  - `pyproject.toml`
  - `setup.py`

## 推荐创建方式

```bash
cd /Users/rain/CTEAS/tutorial
python3 -m venv .venv
source .venv/bin/activate
pip install -U pip
pip install -r requirements.txt
```

或直接使用仓库脚本：

```bash
cd /Users/rain/CTEAS/tutorial
./bootstrap_venv.sh
```

## 当前 requirements

- `numpy==2.4.4`
- `pandas==3.0.2`
- `pyyaml==6.0.3`
- `PySide6==6.11.0`
- `matplotlib==3.10.9`
- `imageio-ffmpeg==0.6.0`

## 说明

- `PySide6` 用于桌面 GUI
- `matplotlib` 用于后处理绘图
- `imageio-ffmpeg` 用于动画导出
- 当前代码未发现必须绑定系统 Python 绝对路径的实现；但部分文档仍残留旧环境绝对路径示例
