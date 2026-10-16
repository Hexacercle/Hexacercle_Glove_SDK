# SDK 直连 MuJoCo：21 节点骨骼示例

`main.py` 用 Python `ctypes` 直接调用本交付包的 XGF SDK，打开手套串口、加载可选的 `.hexprofile`、执行普通 R 快速校准，并在 MuJoCo 窗口中绘制所选手的 21 个骨骼节点及 20 条父子连线。它不需要 Unity、ROS、CAN 或机器人驱动。

连接后控制台会在收到串口回复时打印六位接收器设备 ID。

## 运行

需要 Python、`numpy` 和 `mujoco`；串口须由本程序独占。Windows 使用本包 `接口文档/library/windows-x64` 中的 DLL，Linux 使用 `接口文档/library/linux-x64` 中的 `.so`。以下命令在本示例目录 `examples/mujoco_sdk_skeleton` 执行，端口需替换成实际值。

本目录已放置 `10030.hexprofile` 和 `xgf_skeleton.json`，与 `main.py` 同级。启动时会自动加载这两个文件，与当前工作目录无关；因此不必在命令行重复写 `--profile` 或 `--skeleton`。想使用其他文件时，可显式指定这两个参数；`--no-profile` 可忽略默认 profile。

Windows PowerShell：

```powershell
python main.py --lib ..\..\接口文档\library\windows-x64\HexaCercleGlove.dll --port COM3 --hand both
```

Linux：

```bash
python3 main.py --lib ../../接口文档/library/linux-x64/libHexaCercleGlove.so --port /dev/ttyUSB0 --hand both
```

`--hand left`、`right`、`both` 分别显示左、右或双手。双手在画面中左右分开放置；蓝色为左手实时骨架、橙色为右手实时骨架。未收到该手完整跟踪数据时，其已配置的骨架仍以**灰色**显示，灰色不代表手套正在运动。要看到两只手都动，接收器必须实际提供左右手数据，且校准文件须适用于当前设备与佩戴状态。终端每 5 秒输出 `online_mask` 与左右手 `tracked/waiting`，可据此区分未连接、未跟踪和仅显示静态骨架。

此处的 `10030.hexprofile` 是用户提供的校准数据，仅适用于匹配的手套与佩戴状态。其他开发者应换成自己的 `.hexprofile`。profile 最后加载，可能覆盖前面设定的骨架。

无设备时可先检查 SDK 库和配置文件：

```bash
python3 main.py --lib ../../接口文档/library/linux-x64/libHexaCercleGlove.so --hand both --check
```

`--check` 不打开串口，也不启动 MuJoCo 窗口。它只能验证库加载及配置读取，不能证明手套实测跟踪正常。

## R 快速校准

- **带 profile 启动**：默认使用已加载的校准状态；需要普通 R 时，在 MuJoCo 窗口按 **R**，或启动时加 `--r`。
- **未带 profile 启动**：所选手在线后会自动尝试 2 秒普通 R；也可按 **R** 再执行一次。
- **单手模式**：只对选中的一只手采 R（左手掩码 1、右手掩码 2）。
- **双手模式**：等左右手都在线并有有效输入，再以掩码 3 **同时**开始一次 R；若只连接了一只手，会持续等待，不会把双手 R 拆成两次单手采集。
- R 期间保持手掌平展、稳定。窗口会打印 `sampling`、`complete` 或失败状态；`hex_mocap_update` 在采集期间仍持续运行。

这里使用的是公开的普通 `hex_mocap_start_r_capture`。若重戴后传感器安装零位发生变化，普通 R 不能代替匹配当前佩戴状态的完整校准配置。

## Linux 窗口报错

出现 `GLX: Failed to create context: BadValue` / `could not create window` 时，失败发生在 MuJoCo/GLFW 创建图形窗口阶段。先在**运行示例的同一个桌面终端**检查图形环境：

```bash
echo "DISPLAY=$DISPLAY WAYLAND_DISPLAY=$WAYLAND_DISPLAY XDG_SESSION_TYPE=$XDG_SESSION_TYPE"
python3 main.py --check-gl
glxinfo -B
```

`--check-gl` 不打开串口或加载 SDK，只测试一个不可见的 GLFW 窗口。若系统提示找不到 `glxinfo`，需安装发行版提供的 `mesa-utils`。`glxinfo -B` 也不能正常显示 OpenGL renderer 时，先修复该机器的显示会话、显卡驱动或 GLX 配置。若机器使用 Mesa，可以尝试软件渲染：

```bash
LIBGL_ALWAYS_SOFTWARE=true python3 main.py --lib ../../接口文档/library/linux-x64/libHexaCercleGlove.so --port /dev/ttyUSB0 --hand both
```

软件渲染是诊断和临时运行方式，速度可能较慢；如果依然不能创建窗口，需要查看该机器的 `glxinfo -B` 输出。`MUJOCO_GL=egl`、`osmesa` 用于无窗口渲染，不能让这个交互式 GLFW 窗口凭空出现。参考：[MuJoCo 图形环境说明](https://mujoco.readthedocs.io/en/stable/programming/visualization.html)、[Mesa 软件渲染环境变量](https://docs.mesa3d.org/envvars.html)。

关闭 MuJoCo 窗口或按 Ctrl+C 退出；程序先销毁骨骼会话，再调用 `hex_glove_abort` 停流并关闭串口，最后释放手套会话。不会向手套发送关机指令。
