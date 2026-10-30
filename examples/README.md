# 全部示例编译与测试说明（Linux / Windows）

> **10.27 说明**：以下 2026-10-16 Ubuntu 实机结果属于 10.16 历史测试。本目录已替换 10.27 库；三个 C++ 命令行示例已通过 Windows C++17 编译链接，但本次尚未对 10.27 库重新做真实手套、Qt 或 MuJoCo 实机验收。接口变化见[版本说明](../SDK_CHANGELOG_10.16_TO_10.27.zh-CN.md)。

本文提供本 SDK 全部 7 个可运行示例的编译、运行、自检及退出指令。所有命令均从 **SDK 根目录**执行，不是从 `examples` 目录执行。

## 1. 示例和验证范围

| 示例 | 源码目录 | 功能 | 退出方式 |
| --- | --- | --- | --- |
| Python 快速入门 | `examples/python_quickstart` | 双手骨骼、设备 ID、关节角、CSV/JSON Lines | Ctrl+C |
| C++ 快速入门 | `examples/sdk_quickstart` | 双手骨骼、设备 ID、关节角、CSV/JSON Lines | 约 30 秒自动退出 |
| Python 统一快照 | `examples/external_snapshot/main.py` | 同轮双手骨骼与角度、有效位、诊断、导出 | 按 `--ticks` 自动退出 |
| C++ 统一快照 | `examples/external_snapshot/main.cpp` | 同轮双手骨骼与角度、有效位、诊断、导出 | 约 30 秒自动退出 |
| C++ 连接工作流 | `examples/partner_workflow` | 接收器搜索、连接状态、事件、档案加载 | 按 `--seconds` 自动退出 |
| MuJoCo | `examples/mujoco_sdk_skeleton` | 双手 21 节点三维显示、配置与图形自检 | 关闭窗口或 Ctrl+C |

2026-10-16 实测环境：Ubuntu 22.04 x64、GCC 11.5、Python 3.10、Qt 6.5.3、NumPy 2.2.6、MuJoCo 3.4.0、GLFW 2.10.0；串口 `/dev/ttyUSB0`，接收器 ID `114205`。上述 7 个示例均已实机运行并正常退出，四个控制台角度示例均导出了左右手 CSV/JSON 数据。另验证了自动搜索及 MuJoCo 两种自检。

**Windows 命令按本包源码、参数与 MinGW 导入库编写，尚未在 Windows 实机验证。** 本次 Linux 测试不包括标准角度精度、完整七步校准、拔线恢复或长期稳定性验收。原测试日志与数据未包含在当前发布包中；本包的验证范围及后续接收器 ID 复测事项见[示例复测记录](SDK_EXAMPLE_TEST_REPORT.zh-CN.md)。

## 2. 通用准备

- 每次只运行一个示例；运行前关闭旧上位机、其他示例或串口调试工具，避免串口被占用。
- Python、编译器和 SDK 均使用 64 位版本。本文按 Python 3.10 及以上环境准备，本次实测使用 Python 3.10。
- C++ 使用 C++17。Qt 示例需要 Qt Widgets，支持 Qt5/Qt6；本文以 Qt 6.5.3 为例。
- 普通 Python 示例仅使用标准库；MuJoCo 示例另外需要 `numpy`、`mujoco` 和 `glfw`。
- 示例使用 `接口文档/library/include` 及对应平台的预编译库，头文件与库保持同一套。
- 随包 `10030.hexprofile` 是特定设备、模型及佩戴状态下的档案；用于其他设备时应换成自己的档案。
- 控制台角度示例会在有效姿态上自动采集几何零位，启动时保持手指伸直中立。Qt 由“角度归零”按钮触发。

## 3. Linux 编译与测试

### 3.1 公共准备

在**同一个终端**先执行，后面的命令会引用这些变量。SDK 路径、Qt 路径和串口按实际情况修改。

```bash
cd /home/hexacercle/Downloads/Hexacercle_SDK

SDK_LIB="$PWD/接口文档/library/linux-x64/libHexaCercleGlove.so"
SDK_PROFILE="$PWD/examples/mujoco_sdk_skeleton/10030.hexprofile"
SDK_PORT=/dev/ttyUSB0
SDK_QT=/home/hexacercle/Qt/6.5.3/gcc_64

mkdir -p build-examples test-results/profiles

python3 --version
g++ --version
cmake --version
"$SDK_QT/bin/qmake" -query QT_VERSION
ls -l "$SDK_PORT"
```

仅在缺少 MuJoCo 依赖时安装；如使用 Python 虚拟环境，后续运行也使用该环境：

```bash
python3 -m pip install numpy mujoco glfw
```

### 3.2 编译全部命令行 C++ 示例

```bash
for demo in sdk_quickstart external_snapshot partner_workflow; do
  g++ -std=c++17 -I 接口文档/library/include \
    "examples/$demo/main.cpp" \
    -L 接口文档/library/linux-x64 -lHexaCercleGlove \
    '-Wl,-rpath,$ORIGIN/../接口文档/library/linux-x64' \
    -o "build-examples/$demo" || break
done
```

编译产物放在 `build-examples`。上述运行时搜索路径与这个目录结构配套；`$ORIGIN` 外的单引号需保留，不要让 shell 展开该变量。

### 3.3 Python 快速入门

```bash
python3 examples/python_quickstart/main.py "$SDK_PORT" \
  --lib "$SDK_LIB" --profile "$SDK_PROFILE" \
  --csv test-results/python-quickstart.csv \
  --jsonl test-results/python-quickstart.jsonl
```

按 Ctrl+C 退出并释放串口。

### 3.4 C++ 快速入门

```bash
./build-examples/sdk_quickstart "$SDK_PORT" \
  --profile "$SDK_PROFILE" \
  --csv test-results/cpp-quickstart.csv \
  --jsonl test-results/cpp-quickstart.jsonl
```

约 30 秒后自动退出。

### 3.5 Python 统一快照

```bash
python3 examples/external_snapshot/main.py "$SDK_PORT" \
  --lib "$SDK_LIB" --profile "$SDK_PROFILE" --ticks 80 \
  --csv test-results/python-snapshot.csv \
  --jsonl test-results/python-snapshot.jsonl
```

`--ticks 80` 表示执行 80 轮更新，正常情况下约 8 秒退出；省略时默认 300 轮。

### 3.6 C++ 统一快照

```bash
./build-examples/external_snapshot "$SDK_PORT" \
  --profile "$SDK_PROFILE" \
  --csv test-results/cpp-snapshot.csv \
  --jsonl test-results/cpp-snapshot.jsonl
```

约 30 秒后自动退出。

### 3.7 C++ 连接工作流

此示例接收的是档案**目录**，不是 `--profile` 文件参数。档案名由串口路径的 UTF-8 字节十六进制编码构成；`/dev/ttyUSB0` 对应 `receiver-2f6465762f74747955534230.hexprofile`。下面按当前 `SDK_PORT` 生成文件名并复制档案，不改写原档案。

```bash
SDK_PROFILE_KEY=$(printf '%s' "$SDK_PORT" | od -An -tx1 | tr -d ' \n')
cp "$SDK_PROFILE" "test-results/profiles/receiver-$SDK_PROFILE_KEY.hexprofile"

./build-examples/partner_workflow "$SDK_PORT" \
  "$PWD/test-results/profiles" --seconds 8
```

自动搜索接收器：

```bash
./build-examples/partner_workflow auto \
  "$PWD/test-results/profiles" --seconds 8
```

自动搜索可能选择 `/dev/serial/by-id/...`，与 `/dev/ttyUSB0` 是不同的档案键。若日志为 `profile=0`，应按日志中的实际 `port=` 路径另行复制档案。示例：

```bash
# 把此处路径替换成日志显示的实际接收器路径。
SDK_DETECTED_PORT=/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0
SDK_PROFILE_KEY=$(printf '%s' "$SDK_DETECTED_PORT" | od -An -tx1 | tr -d ' \n')
cp "$SDK_PROFILE" "test-results/profiles/receiver-$SDK_PROFILE_KEY.hexprofile"

./build-examples/partner_workflow auto \
  "$PWD/test-results/profiles" --seconds 8
```

有双手新鲜数据时应看到 `phase=4`、`streaming=3`；档案加载后为 `profile=1`。

### 3.8 MuJoCo

检查 SDK 与配置，不打开串口或图形窗口：

```bash
python3 examples/mujoco_sdk_skeleton/main.py \
  --lib "$SDK_LIB" --hand both --check
```

只检查 GLFW/OpenGL 环境，不打开串口：

```bash
python3 examples/mujoco_sdk_skeleton/main.py --check-gl
```

连接手套并显示双手骨骼：

```bash
python3 examples/mujoco_sdk_skeleton/main.py \
  --lib "$SDK_LIB" --port "$SDK_PORT" --hand both
```

默认自动加载源码旁边的 `10030.hexprofile` 和 `xgf_skeleton.json`，与工作目录无关。改用其他档案时加 `--profile "$SDK_PROFILE"`；`--hand left/right/both` 可选择显示的手。关闭窗口或按 Ctrl+C 退出。

## 4. Windows 编译与测试（PowerShell）

### 4.1 开发环境与公共准备

准备 Qt 6.5.x **MinGW 64 位套件**、与该套件配套的 MinGW 编译器、CMake 和 64 位 Python。现有 `libHexaCercleGlove.dll.a` 是 MinGW 导入库，不能直接作为 MSVC 的 `.lib` 使用；以下命令不适用于 Qt MSVC 套件。

SDK 在设备管理器中应有可用 COM 端口。关闭占用该端口的旧程序。下面的 `C:\SDK\Hexacercle_SDK`、Qt 路径、MinGW 路径和 `COM3` 均为示例，请改成自己的实际值。

在**同一个 PowerShell 窗口**先执行：

```powershell
Set-Location "C:\SDK\Hexacercle_SDK"

$SdkRoot = (Get-Location).Path
$SdkBin = Join-Path $SdkRoot "接口文档\library\windows-x64"
$SdkLib = Join-Path $SdkBin "HexaCercleGlove.dll"
$SdkProfile = Join-Path $SdkRoot "examples\mujoco_sdk_skeleton\10030.hexprofile"
$SdkPort = "COM3"
$SdkQt = "C:\Qt\6.5.3\mingw_64"
$SdkMingw = "C:\Qt\Tools\mingw1120_64\bin"
$env:PATH = "$SdkQt\bin;$SdkMingw;$SdkBin;$env:PATH"

New-Item -ItemType Directory -Force build-examples-win, test-results, test-results\profiles | Out-Null

python --version
python -c "import struct; print('Python 位数:', struct.calcsize('P') * 8)"
g++ --version
g++ -dumpmachine
cmake --version
& "$SdkQt\bin\qmake.exe" -query QT_VERSION
Get-Item $SdkLib
```

确认 Python 为 64 位，`g++ -dumpmachine` 对应 x86_64；不要混用不同架构或不匹配的编译器运行时。运行时需要 `HexaCercleGlove.dll`、`libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`，本包已将其放在同一目录。

仅在缺少 MuJoCo 依赖时安装，使用与运行示例相同的 Python 环境：

```powershell
python -m pip install numpy mujoco glfw
```

### 4.2 编译全部命令行 C++ 示例

```powershell
foreach ($demo in @("sdk_quickstart", "external_snapshot", "partner_workflow")) {
    g++ -std=c++17 -I ".\接口文档\library\include" `
        ".\examples\$demo\main.cpp" `
        -L ".\接口文档\library\windows-x64" -lHexaCercleGlove `
        -o ".\build-examples-win\$demo.exe"
    if ($LASTEXITCODE -ne 0) { throw "编译失败：$demo" }
}
```

PowerShell 多行命令的反引号必须位于行尾，后面不要添加空格。公共准备中已把 SDK DLL 目录加入当前终端的 PATH；从其他环境运行时也需要配置 DLL 搜索路径。

### 4.3 Python 快速入门

```powershell
python .\examples\python_quickstart\main.py $SdkPort `
    --lib "$SdkLib" --profile "$SdkProfile" `
    --csv .\test-results\python-quickstart.csv `
    --jsonl .\test-results\python-quickstart.jsonl
```

按 Ctrl+C 退出。

### 4.4 C++ 快速入门

```powershell
.\build-examples-win\sdk_quickstart.exe $SdkPort `
    --profile "$SdkProfile" `
    --csv .\test-results\cpp-quickstart.csv `
    --jsonl .\test-results\cpp-quickstart.jsonl
```

约 30 秒后自动退出。

### 4.5 Python 统一快照

```powershell
python .\examples\external_snapshot\main.py $SdkPort `
    --lib "$SdkLib" --profile "$SdkProfile" --ticks 80 `
    --csv .\test-results\python-snapshot.csv `
    --jsonl .\test-results\python-snapshot.jsonl
```

80 轮更新后退出，正常情况下约 8 秒。

### 4.6 C++ 统一快照

```powershell
.\build-examples-win\external_snapshot.exe $SdkPort `
    --profile "$SdkProfile" `
    --csv .\test-results\cpp-snapshot.csv `
    --jsonl .\test-results\cpp-snapshot.jsonl
```

约 30 秒后自动退出。

### 4.7 C++ 连接工作流

根据实际串口名生成档案键；例如 `COM3` 对应 `434f4d33`。此处只复制原档案。

```powershell
$SdkProfileKey = [BitConverter]::ToString([Text.Encoding]::UTF8.GetBytes($SdkPort)).Replace("-", "").ToLowerInvariant()
Copy-Item "$SdkProfile" ".\test-results\profiles\receiver-$SdkProfileKey.hexprofile"

.\build-examples-win\partner_workflow.exe $SdkPort `
    "$SdkRoot\test-results\profiles" --seconds 8
```

自动搜索接收器：

```powershell
.\build-examples-win\partner_workflow.exe auto `
    "$SdkRoot\test-results\profiles" --seconds 8
```

自动搜索选中的端口与 `$SdkPort` 不同时，应按日志实际端口名重新准备对应档案。

### 4.8 MuJoCo

SDK 与配置自检：

```powershell
python .\examples\mujoco_sdk_skeleton\main.py `
    --lib "$SdkLib" --hand both --check
```

图形环境自检：

```powershell
python .\examples\mujoco_sdk_skeleton\main.py --check-gl
```

真实连接并显示双手骨骼：

```powershell
python .\examples\mujoco_sdk_skeleton\main.py `
    --lib "$SdkLib" --port $SdkPort --hand both
```

默认自动读取脚本旁边的档案和骨架配置；关闭窗口或按 Ctrl+C 退出。

部署后仍需保留配套 SDK DLL，且实际设备驱动、运行时兼容性及完整功能应在目标 Windows 机器验证。

## 5. 参数、输出与结果判断

### 5.1 常用参数

| 参数 | 适用示例 | 含义 |
| --- | --- | --- |
| `--profile 文件` | 快速入门、统一快照、MuJoCo、Qt | 加载已有校准档案 |
| `--lib 文件` | Python、MuJoCo | 指定原生 SDK 动态库 |
| `--csv 文件` | 快速入门、统一快照、Qt | 导出角度 CSV |
| `--jsonl 文件` | 快速入门、统一快照、Qt | 导出角度 JSON Lines |
| `--jsonl -` | 上述支持 JSON Lines 的示例 | 将逐帧 JSON 写到标准输出 |
| `--r` | 控制台快速入门、统一快照、连接工作流、MuJoCo | 加载档案后仍执行普通 R |
| `--hand left/right/both` | 连接工作流、MuJoCo | 选择 R 或显示的手 |
| `--seconds N` | 连接工作流 | 运行约 N 秒 |
| `--ticks N` | Python 统一快照 | 执行 N 轮更新 |

需要重新普通 R 时，在对应命令末尾加 `--r`；工作流可加 `--r --hand both`。R 期间保持手掌平直稳定。Qt 通过界面按钮执行 R。传感器 R 与几何角度零位不同，重新 R 后应重新建立角度零位。普通 R 不能替代匹配佩戴状态的完整校准。

### 5.2 如何判断测试结果

- 设备 ID 应从等待回复变为六位标识；本次接收器为 `114205`。
- 每手骨骼 `tracked_mask=001fffff`，或十进制 `2097151`，表示 21 节点都在跟踪。仅 `valid_mask` 有值不代表实时跟踪。
- 快照 `online/tracked/angles` 为 `3` 时表示左右手对应位均有效；输出应同时检查新帧有效位。
- 工作流 `phase=4`、`streaming=3` 表示双手数据流；`profile=1` 表示已加载档案。
- CSV/JSON 应有左右手记录、时间戳、帧号和 15 个角度字段，单位为度。已有文件可能被同名运行覆盖。
- MuJoCo 日志应出现 `online_mask=3`、`left:tracked right:tracked`；灰色模型不能作为实时跟踪证据。
- 界面显示的观测帧率、双手到达间隔不等于硬件采样率、端到端延迟或硬件同步证明。

### 5.3 常见问题

| 现象 | 检查方式 |
| --- | --- |
| 串口打开失败 | 核对实际端口、关闭占用程序、检查驱动；Linux 确认当前用户有串口权限 |
| Linux 找不到 `.so` | 使用上述配套目录；Python 传绝对 `--lib` 路径，C++ 保留 rpath 或配置 `LD_LIBRARY_PATH` |
| Windows 找不到 DLL / 错误 126 | 确认主 DLL 和依赖齐全，SDK 目录在搜索路径，Python 和 DLL 均为 64 位 |
| Windows 架构错误 / 错误 193 | 检查 Python、MinGW、Qt、SDK 是否同为 x64 |
| MuJoCo 无法创建图形窗口 | 先执行 `--check-gl`；Linux 在本地图形终端检查显示会话和 OpenGL 环境 |
| 工作流 `profile=0` | 按实际选中的串口路径生成档案键，不要把 COM 端口或 Linux 别名当成同一个键 |
| 没有角度导出 | 检查是否有新鲜且完整的跟踪帧、R 是否就绪、角度零位是否建立 |

详细 API 与生命周期见 [接口文档](../接口文档/README.zh-CN.md)，示例原理见 [SDK 使用说明](SDK_USAGE_PYTHON_CPP_QT.zh-CN.md)，MuJoCo 详细说明见 [MuJoCo README](mujoco_sdk_skeleton/README.zh-CN.md)。
