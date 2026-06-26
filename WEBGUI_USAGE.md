# WebGUI Removed

`webgui` 和 `streamlitgui` 入口已移除。

## 替代方式

桌面 GUI：

```bash
cd /home/ai/tutorial
./calpuff-cli desktopgui
```

CLI 编排：

```bash
cd /home/ai/tutorial
./calpuff-cli run --case smoke5d --stage all
```

如果执行 `./calpuff-cli webgui` 或 `./calpuff-cli streamlitgui`，现在会直接返回明确错误，不再启动本地 Web 服务。
