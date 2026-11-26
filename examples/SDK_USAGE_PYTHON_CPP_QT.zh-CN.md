# SDK 使用示例：Python / C++ / Qt

Windows 使用 11.20-MSVC x64。Linux x64 沿用 10.16 的单接收器库，编译时使用 `接口文档/library/linux-x64/include`。双有线串口、Q17 七步和对指吸附适用于 Windows Qt。

需要 SDK 管理接收器搜索、自动重连、状态事件及 profile 自动加载的示例见 [Qt 骨骼查看器](qt_skeleton_viewer/main.cpp)。它使用 `hex_partner_tick` 推进一次更新，再读取左右手 21 节点。快速 R 在双手都输出数据时同时采集双手的佩戴零位；完整多步校准的调用顺序见[上位机对接指南](../接口文档/PARTNER_INTEGRATION.zh-CN.md)。

无线接收器打开后 SDK 自动请求设备 ID；Python、C++、Qt 与 MuJoCo 示例均使用 `hex_glove_get_device_id` 在回复到达后读取六位 ID。Qt 顶部状态栏显示该 ID；若尚未收到回复则显示“等待设备回复”。 三个命令行示例现先等 ID 回复，再查询信道；ID 未在限定时间内返回时会补发读 ID 指令并继续查询信道。测试范围与自动搜索的档案绑定说明见[示例复测记录](SDK_EXAMPLE_TEST_REPORT.zh-CN.md)。

## 新增：统一单帧快照（推荐外部接入）

[C++ 源码](external_snapshot/main.cpp)与[Python 源码](external_snapshot/main.py)展示连接、profile、普通 R、左右手 21 节点、同一轮更新得到的 15 项 Hub 角度、状态/接收诊断、ABI 检查、错误信息、CSV 与 JSON Lines 实时输出。`fresh_hand_mask` 用于跳过本轮没有新设备帧的手。Qt 可在定时器回调中采用相同的 `hex_external_update → hex_external_get_frame` 流程，把 `frame.pose[hand]` 交给已有渲染逻辑；请勿在一轮 Qt 回调中再额外调用一次 `hex_mocap_update`。

Linux 编译 C++：`g++ -std=c++17 -I 接口文档/library/linux-x64/include examples/external_snapshot/main.cpp -L 接口文档/library/linux-x64 -lHexaCercleGlove -o external_snapshot`。运行前设置 `LD_LIBRARY_PATH` 指向 `linux-x64`。Windows 使用 Visual Studio 2022 MSVC x64，链接 `windows-x64/HexaCercleGlove.lib`，将主 DLL 与随附的 VC 运行库放到可搜索路径。

Python：`python3 examples/external_snapshot/main.py /dev/ttyUSB0 --lib 接口文档/library/linux-x64/libHexaCercleGlove.so --profile your.hexprofile --csv angles.csv --jsonl -`。Windows 将端口改为 `COM3`、库改为对应 DLL。`--r` 表示加载 profile 后仍重新执行普通 R；此后示例会在新鲜姿态上校准角度零位。未提供 profile 时使用合成骨架，仅用于验证接入链路。

MuJoCo 直连骨骼显示、profile 加载和普通 R 的独立示例见 [mujoco_sdk_skeleton](mujoco_sdk_skeleton/README.zh-CN.md)。

这三个示例面向当前 `HexaCercleGlove` 原生 C API，均演示：打开串口手套、查询左右手状态及基础数据、创建 mocap 会话、读取 21 节点骨骼、校准并获取逐关节角。C++、Python 和 Qt 均可按帧输出带时间戳的 CSV 与 JSON Lines；`--jsonl -` 将 JSON Lines 写到标准输出，供其他进程实时消费。Qt 使用 Unity 的待机姿态和关节映射，也可加载匹配的档案。画布把 SDK 的 21 节点世界位置投影成二维线框。

## 准备

1. 从本包的 `接口文档/library/windows-x64` 或 `接口文档/library/linux-x64` 选择对应平台的预编译库；Windows C++/Qt 使用 `接口文档/library/include`，Linux C++/Qt 使用 `接口文档/library/linux-x64/include` 头文件。
2. 确认串口，如 Windows `COM3`、Linux `/dev/ttyUSB0`，并确保没有其他程序占用。
3. Windows 的 Python、Qt、编译器及 SDK 均需使用 x64；本包 Windows 链接库适用于 MSVC x64，Qt 使用 MSVC 套件。
4. Windows Qt 可连接一个无线接收器或按左、右指定两个有线数据口；Linux 使用单接收器和该平台的配套库、头文件。

## C++ 命令行示例

源码：[sdk_quickstart/main.cpp](sdk_quickstart/main.cpp)。在本包根目录执行：

Linux：

```sh
g++ -std=c++17 -I 接口文档/library/linux-x64/include examples/sdk_quickstart/main.cpp -L 接口文档/library/linux-x64 -lHexaCercleGlove -Wl,-rpath,'$ORIGIN/接口文档/library/linux-x64' -o sdk_quickstart
./sdk_quickstart /dev/ttyUSB0
```

Windows（Visual Studio 2022 MSVC x64，PowerShell）：

```powershell
cmake -S examples -B build-examples-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-examples-msvc --config Release --parallel
.\build-examples-msvc\Release\sdk_quickstart.exe COM3
```

该命令同时构建 `external_snapshot.exe` 和 `partner_workflow.exe`，并将 SDK DLL 及随附 VC 运行库复制到 `Release` 目录。手工集成时配置 C++17、`/utf-8`、`library/include`，链接 `library/windows-x64/HexaCercleGlove.lib`。

已有校准文件时可运行 `.\build-examples-msvc\Release\sdk_quickstart.exe COM3 --profile .\my.hexprofile`；需要重新执行普通 R 时再加 `--r`。加 `--csv angles.csv --jsonl angles.jsonl` 同时写两种文件；`--jsonl -` 将每帧 JSON 写到标准输出。程序约每 100 ms 更新一次，持续约 30 秒；打印连接、布局、电量、路由、接收字节、骨骼掌节点和部分角度。`valid_mask` 表示模型节点存在，`tracked_mask` 表示当前跟踪有效。

## Python 示例

源码：[python_quickstart/main.py](python_quickstart/main.py)。仅用 Python 标准库 `ctypes`，不依赖额外包：

```powershell
python examples/python_quickstart/main.py COM3 --lib .\接口文档\library\windows-x64\HexaCercleGlove.dll
```

```sh
python3 examples/python_quickstart/main.py /dev/ttyUSB0 --lib ./接口文档/library/linux-x64/libHexaCercleGlove.so
```

两个平台都可加 `--profile 文件路径` 加载已有校准文件；需要普通 R 时再加 `--r`，需要导出时加 `--csv angles.csv --jsonl angles.jsonl`。`--jsonl -` 可用管道实时消费 JSON；状态文字写到标准错误，避免混入数据流。按 Ctrl+C 断开。示例打印连接状态、电量、信道、骨骼和逐关节角。`ctypes.Structure` 使用头文件的默认对齐，不能添加 `_pack_=1`；本版的 Hub 配置与状态结构大小已和 C++ ABI 核对。升级 SDK 结构时要同步这些定义。

## Qt 完整上位机示例

源码：[qt_skeleton_viewer/main.cpp](qt_skeleton_viewer/main.cpp)，独立 [CMakeLists.txt](qt_skeleton_viewer/CMakeLists.txt)。Windows 使用 Qt 6.5.3 MSVC x64 Release，并排显示 SDK 骨骼线框和 Unity FBX 三维模型。随包运行版可直接启动。Linux 线框示例使用 Qt Widgets（Qt 5 或 Qt 6）。[Windows 运行版](qt_skeleton_viewer/bin/qt_skeleton_viewer.exe)

Qt 窗口展示接收器阶段、最新操作状态和失败码、串口失败阶段／系统码、双手在线／新帧位、主机侧频率与双手到达差。按钮可断开／重连、暂停／恢复数据流、单手或双手快速 R（2 秒）、启动／取消／重试 Unity 七步标定及重采当前对指、保存／加载按串口绑定的 profile。四个既有 Unity UDP JSON 通道在收到新帧时仅预览编码结果；示例不会自动发送网络报文。需要真正广播时由接入方把编码缓冲发往目标 UDP 地址。无 Qt SDK 的环境无法本机编译图形窗口，应在已安装 Qt Widgets 的目标环境执行下列命令。

Windows Qt 支持从默认 M11 映射或兼容档案开始新版七步。它先采集平放、伸直、握拳，再对食、中、无名、小指各采一次稳定的指腹捏和指尖掐，中间可以松开。等待所选双手分别通过端点检查。Linux Qt 使用原连续运动流程，需先加载与佩戴匹配的档案。

骨骼画布使用 SDK `hand=0` 左、`hand=1` 右。腕部 XY 锁定时，画布沿当前掌面投影。开启时使用捕获的固定观察轴，可以看到腕部转动。按 F8 可按当前双手掌面重置观察方向。Qt 中 **F5 或 F6** 均按 Unity 的 F6 逻辑切换双手腕部 X/Y 同步：读取 `hex_mocap_get_pipeline_config`，只修改 `model_driver.node[0].sync_axis_mask` 的低两位，再写回 `hex_mocap_set_pipeline_config`；Z 轴配置保持原值。手套没有外部空间定位，画布不显示手腕的绝对平移。Unity `_XGF` 工程的 F5 用于指尖距离显示。

Qt 使用 5 毫秒精确定时器轮询 SDK；`左手/右手 帧/秒` 取自 `hex_partner_get_timing().observed_hz`，表示此程序实际观察到的新帧频率，**不是硬件标称采样率**。旧版 33 毫秒轮询把该数值人为限制在约 30 Hz。改短轮询后，实际数值仍取决于手套发送频率、串口链路、系统调度和界面处理时间。

```sh
cmake -S examples/qt_skeleton_viewer -B build-qt-demo -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt-demo --target qt_skeleton_viewer
./build-qt-demo/qt_skeleton_viewer /dev/ttyUSB0 --no-profile
```

Windows 使用 Qt 6.5.3 MSVC x64 Release。

```bat
cmake -S examples/qt_skeleton_viewer -B build-qt-msvc -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64
cmake --build build-qt-msvc --config Release --target qt_skeleton_viewer
set "PATH=C:\Qt\6.5.3\msvc2019_64\bin;%PATH%"
set "QT_QPA_PLATFORM_PLUGIN_PATH=C:\Qt\6.5.3\msvc2019_64\plugins\platforms"
build-qt-msvc\Release\qt_skeleton_viewer.exe COM3 --no-profile
build-qt-msvc\Release\qt_skeleton_viewer.exe --wired COM12 COM14 --no-profile
```

双有线示例中的 COM12、COM14 依次代表左、右数据口，按实际设备替换。不带串口参数时可在界面选择后连接。`--no-profile` 停用本次档案读写，Windows 的两秒快速 R 和七步结果仍可在会话内使用。`--profile 文件路径` 显式加载档案。省略档案参数时，无线按接收器串口匹配，有线按有序的左右串口组合匹配。

双手平放、手指自然伸直，点击 **R 快速校准（2 秒）**。R 完成后点击 **角度归零（手指伸直）** 查看屈伸角。可加 `--csv angles.csv` 与 `--jsonl angles.jsonl` 导出结果。线框画布使用 SDK 的世界坐标和当前模型的父节点连接。Windows 的三维画布将同一 SDK 姿态提交给 `HexQtFbxView` 控件。

Windows Qt 默认关闭对指吸附。在完整示例窗口按住 `Ctrl` 和 `X`，再按一下 `F` 切换双手吸附。校准期间暂停切换。切换结果见“事件记录”。开启后会在支持的档案上应用指尖接触辅助，关闭后沿用逐指校准结果。切换连接时保留当前选择，校准文件不存储该开关。

## Windows Qt FBX 最小接入示例

`qt_fbx_embed` 使用 Windows SDK 提供的预编译 Qt 控件驱动双手模型。使用 **Qt 6.5.3 MSVC x64 Release**，并把示例中的 Qt 安装目录改成实际路径。首次无参数启动即可查看待机姿态。

```powershell
$SdkQt = "C:\Qt\6.5.3\msvc2019_64"
$env:PATH = "$SdkQt\bin;$env:PATH"
cmake -S examples/qt_fbx_embed -B build-qt-fbx -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=$SdkQt"
cmake --build build-qt-fbx --config Release --target qt_fbx_embed
.\build-qt-fbx\Release\qt_fbx_embed.exe
```

无线接收器可用 `qt_fbx_embed.exe COM3`，有线模式可用 `qt_fbx_embed.exe --wired COM12 COM14`，双有线参数顺序为左、右。换用设备管理器中的实际数据口。收到数据后，保持平掌、手指自然伸直，在窗口内按 `R`，完成两秒校准后可看到跟踪模型。

示例只通过 `hex_partner_tick` 推进 SDK，并从 `hex_external_get_frame` 读取同轮结果。按手检查 `fresh_hand_mask`、`tracked_hand_mask` 和 `pose[hand].tracked_mask` 后再向控件提交姿态。组件调用、Qt 依赖部署和自有应用接入见 [Qt FBX 接入说明](../接口文档/QT_FBX_INTEGRATION.zh-CN.md) 和 [最小示例源码](qt_fbx_embed/main.cpp)。

## 逐关节角与实时输出

三个示例用 `hex_angle_hub_config_init` / `hex_angle_hub_state_init` 为左右手分别创建 21 槽角度状态，完成 R 后以最终 `HexMocapDisplayPose` 调用 `hex_angle_hub_calibrate`，每个新帧调用 `hex_angle_hub_update`。输出到 CSV/JSON 的 `thumb_cmc`、`thumb_mcp`、`thumb_ip` 和四指 MCP/PIP/DIP 都来自同一 Hub 算法，单位为度。示例也实际调用 `hex_mocap_calibrate_angles` 与 `hex_mocap_get_precise_angles`，在状态输出中显示 `finger[5][4]` 的拇指和食指 MCP 精确角及其自带时间戳；该数组没有单独的 CMC 项，故不把两套角度拼成同一组字段。C++/Python 在首个完整有效帧建立几何角零位，启动时请保持手指伸直中立；Qt 由按钮明确触发。没有有效的完整跟踪、重复帧或尚未完成 R 时，不写新角度行。

每行包含 `timestamp_ms`（SDK 单调时间，毫秒）、`utc_timestamp_ms`（宿主 UTC Unix 毫秒）、`frame_sequence`、`hand` 和 15 个角度字段。CSV 表头与 JSON 键名相同：

```text
thumb_cmc,thumb_mcp,thumb_ip,index_mcp,index_pip,index_dip,middle_mcp,middle_pip,middle_dip,ring_mcp,ring_pip,ring_dip,little_mcp,little_pip,little_dip
```

实时消费例子：`python3 examples/python_quickstart/main.py /dev/ttyUSB0 --profile my.hexprofile --lib ./接口文档/library/linux-x64/libHexaCercleGlove.so --jsonl - | your_consumer`。接收方按换行读取每条 JSON。CSV/JSON 的文件写入和传输由示例程序完成；SDK 提供带时间戳的骨骼和关节角结果。

## 关键接口与注意事项

| 步骤 | 函数 | 说明 |
| --- | --- | --- |
| 建立连接 | `hex_glove_create` → `hex_glove_start` | 端口名是 UTF-8。`start` 返回 0 表示失败。 |
| 基础状态 | `hex_glove_get_device_snapshot`、`hex_glove_get_rx_bytes` | 读取连接、布局、电量、路由和收包量；字段有效性还要结合在线位判断。 |
| 建立求解 | `hex_mocap_create`、`hex_mocap_skeleton_definition_init`、`hex_mocap_set_skeleton_definition` | 示例采用 SDK 合成手模型，供链路和画线验证。正式应用应上传与实际显示模型一致的 rest 骨架。 |
| 加载已有校准 | `hex_mocap_profile_load` | 使用 `--profile` 时，在打开串口前加载；不要再上传演示骨架覆盖档案模型。 |
| R 与逐帧求解 | `hex_mocap_start_r_capture`、`hex_mocap_update` | R 依靠后续新帧推进；无有效手数据时不能开始。也可用 `hex_mocap_get_r_capture_info` 显示进度。 |
| 获取骨骼 | `hex_mocap_get_display_pose` | 输出 21 节点世界/局部 TRS；先设置 `struct_size`，再检查 `valid_mask` 和 `tracked_mask`。单位为米，四元数 `(x,y,z,w)`。 |
| 关节角 | `hex_angle_hub_calibrate`、`hex_angle_hub_update` | 单独建立伸直中立位几何零点，逐帧读取 `state.flexion[21]`；拇指 CMC 位于槽 1。 |
| 精确角 | `hex_mocap_calibrate_angles`、`hex_mocap_get_precise_angles` | 读取 `finger[5][4]` 和精确角快照时间戳；没有单独的拇指 CMC 槽位。 |
| 释放 | `hex_mocap_destroy` → `hex_glove_abort` → `hex_glove_destroy` | mocap 不拥有 glove；`shutdown` 会让设备关机，仅在明确需要时调用。 |

内部 SDK 另有重戴 R 和原 UI 七步校准入口，当前外部接入示例仍只使用约定的普通 R 与骨骼接口。

`HexMocapDisplayPose` 的世界坐标与旋转适合绘图/定位；向 Unity/Qt 模型设置 `localRotation` 时，应使用局部旋转字段或按父骨骼世界旋转换算。控制台示例可使用合成骨架验证数据流。使用 Windows 的 FBX 控件可查看与配套 Unity 手骨架绑定的三维模型。详细契约见 [对外 API 说明](../接口文档/EXTERNAL_API.zh-CN.md)。





