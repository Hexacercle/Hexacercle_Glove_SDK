# 全部示例编译与测试说明（Linux / Windows）

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
| Qt 完整上位机 | `examples/qt_skeleton_viewer` | Windows 显示骨骼线框和 FBX，提供角度、校准及事件。Linux 显示骨骼线框 | 关闭窗口 |
| Qt FBX 最小接入 | `examples/qt_fbx_embed` | Windows 三维手部控件、待机预览、串口跟踪和快速 R | 关闭窗口 |

10.16 原包的历史实测环境（2026-10-16）：Ubuntu 22.04 x64、GCC 11.5、Python 3.10、Qt 6.5.3、NumPy 2.2.6、MuJoCo 3.4.0、GLFW 2.10.0；串口 `/dev/ttyUSB0`，接收器 ID `114205`。当时的 7 个示例均已实机运行并正常退出，四个控制台角度示例均导出了左右手 CSV/JSON 数据。另验证了自动搜索及 MuJoCo 两种自检。

本版 Windows 示例链接 11.20-MSVC x64 库。Linux 保留 10.16 的单接收器库及平台头文件，使用 `接口文档/library/linux-x64/include`。本版验证范围见[发布说明](../README.md)，10.16 的实机情况见[历史示例复测记录](SDK_EXAMPLE_TEST_REPORT.zh-CN.md)。

## 2. 通用准备

- 每次只运行一个示例；运行前关闭旧上位机、其他示例或串口调试工具，避免串口被占用。
- Python、编译器和 SDK 均使用 64 位版本。本文按 Python 3.10 及以上环境准备，10.16 历史测试使用 Python 3.10。
- C++ 使用 C++17。Windows Qt 完整示例和 FBX 最小示例使用 Qt 6.5.3 MSVC x64 Release。Linux 线框示例使用 Qt Widgets（Qt 5 或 Qt 6）。
- 普通 Python 示例仅使用标准库；MuJoCo 示例另外需要 `numpy`、`mujoco` 和 `glfw`。
- Windows 示例使用 `接口文档/library/include`；Linux 使用 `接口文档/library/linux-x64/include`。头文件与对应平台的预编译库保持同一套。
- 随包 `10030.hexprofile` 仅保留作旧 Linux 示例。Windows 请从默认映射或兼容的个人档案开始。
- 控制台角度示例会在有效姿态上自动采集几何零位，启动时保持手指伸直中立。Qt 由“角度归零”按钮触发。

## 3. Linux 编译与测试

Linux `.so` 与其配套头文件沿用 10.16 原包；原版头文件另存于 `接口文档/library/linux-x64/include`，下列 Linux 命令使用此路径。本次未重新编译或验证 Linux。

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
  g++ -std=c++17 -I 接口文档/library/linux-x64/include \
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

### 3.9 Qt 6.5.3

```bash
cmake -S examples/qt_skeleton_viewer -B build-qt65 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$SDK_QT" \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=TRUE

cmake --build build-qt65 -j2

./build-qt65/qt_skeleton_viewer "$SDK_PORT" --no-profile
```

禁用 Qt5 回退可避免 Qt6 未找到时误用 Qt5；已经找到 Qt6 时，CMake 可能提示该回退变量未使用，不影响构建。改变 Qt 版本、编译器或操作系统时使用新的构建目录。

Qt 顶部显示设备 ID、双手数据状态和观测帧率。双手平放、手指自然伸直，点击“R 快速校准（2 秒）”采集佩戴零位。随后点击“角度归零”查看角度；需要导出时在启动命令末尾加 `--csv test-results/qt-angles.csv --jsonl test-results/qt-angles.jsonl`。关闭窗口释放串口。

## 4. Windows 编译与测试（PowerShell）

### 4.1 开发环境与公共准备

准备 Visual Studio 2022（含“使用 C++ 的桌面开发”和 Windows SDK）、CMake、64 位 Python。完整 Qt 示例和 FBX 最小示例使用 Qt 6.5.3 MSVC x64 Release。下例中 `msvc2019_64` 可由 VS2022 工具链使用。Windows 三维显示需要支持 OpenGL 3.3 的驱动。

SDK 在设备管理器中应有可用 COM 端口。关闭占用该端口的旧程序。下面的 `C:\SDK\Hexacercle_SDK`、Qt 路径和 `COM3` 均为示例，请改成自己的实际值。

在**同一个 PowerShell 窗口**先执行以下命令。首次运行可以保持 `$SdkProfile = $null`。控制台示例会使用演示骨架，Windows Qt 可使用默认映射开始个人校准。

```powershell
Set-Location "C:\SDK\Hexacercle_SDK"

$SdkRoot = (Get-Location).Path
$SdkBin = Join-Path $SdkRoot "接口文档\library\windows-x64"
$SdkLib = Join-Path $SdkBin "HexaCercleGlove.dll"
# 可选：填入本机已经验证兼容且匹配佩戴的个人档案路径。
$SdkProfile = $null
$SdkProfileArgs = @()
if ($SdkProfile) { $SdkProfileArgs = @("--profile", $SdkProfile) }
$SdkPort = "COM3"
$SdkQt = "C:\Qt\6.5.3\msvc2019_64"
$env:PATH = "$SdkQt\bin;$SdkBin;$env:PATH"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = "$SdkQt\plugins\platforms"

New-Item -ItemType Directory -Force test-results, test-results\profiles | Out-Null

python --version
python -c "import struct; print('Python 位数:', struct.calcsize('P') * 8)"
cmake --version
& "$SdkQt\bin\qmake.exe" -query QT_VERSION
Get-Item $SdkLib
```

确认 Python 为 64 位，CMake 使用 `-A x64`。本包 DLL 使用 MSVC Release 动态运行库（`/MD`）；运行时需要主 DLL 和 `windows-x64` 内随附的 VC 运行库 DLL。CMake 示例会复制这些 DLL 到程序目录；在其他应用中应同样部署，或安装匹配/更新的 Microsoft Visual C++ x64 Redistributable。

仅在缺少 MuJoCo 依赖时安装，使用与运行示例相同的 Python 环境：

```powershell
python -m pip install numpy mujoco glfw
```

### 4.2 编译全部命令行 C++ 示例

```powershell
cmake -S .\examples -B .\build-examples-msvc -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { throw "示例配置失败" }
cmake --build .\build-examples-msvc --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "示例编译失败" }
```

三份程序及 SDK/VC 运行库 DLL 位于 `build-examples-msvc/Release`。构建脚本使用 C++17、`/utf-8`，链接随包 `HexaCercleGlove.lib`。普通 PowerShell 即可通过 Visual Studio CMake 生成器构建，无需将 `cl.exe` 手动加入 PATH。切换编译器时使用新的构建目录。

### 4.3 Python 快速入门

```powershell
python .\examples\python_quickstart\main.py $SdkPort `
    --lib "$SdkLib" @SdkProfileArgs `
    --csv .\test-results\python-quickstart.csv `
    --jsonl .\test-results\python-quickstart.jsonl
```

按 Ctrl+C 退出。

### 4.4 C++ 快速入门

```powershell
.\build-examples-msvc\Release\sdk_quickstart.exe $SdkPort `
    @SdkProfileArgs `
    --csv .\test-results\cpp-quickstart.csv `
    --jsonl .\test-results\cpp-quickstart.jsonl
```

约 30 秒后自动退出。

### 4.5 Python 统一快照

```powershell
python .\examples\external_snapshot\main.py $SdkPort `
    --lib "$SdkLib" @SdkProfileArgs --ticks 80 `
    --csv .\test-results\python-snapshot.csv `
    --jsonl .\test-results\python-snapshot.jsonl
```

80 轮更新后退出，正常情况下约 8 秒。

### 4.6 C++ 统一快照

```powershell
.\build-examples-msvc\Release\external_snapshot.exe $SdkPort `
    @SdkProfileArgs `
    --csv .\test-results\cpp-snapshot.csv `
    --jsonl .\test-results\cpp-snapshot.jsonl
```

约 30 秒后自动退出。

### 4.7 C++ 连接工作流

无线示例按实际接收器端口名匹配档案，例如 `COM3` 对应 `434f4d33`。提供了兼容档案时才复制到本次示例的档案目录。

```powershell
$SdkProfileKey = [BitConverter]::ToString([Text.Encoding]::UTF8.GetBytes($SdkPort)).Replace("-", "").ToLowerInvariant()
if ($SdkProfile) { Copy-Item -LiteralPath $SdkProfile -Destination ".\test-results\profiles\receiver-$SdkProfileKey.hexprofile" }

.\build-examples-msvc\Release\partner_workflow.exe $SdkPort `
    "$SdkRoot\test-results\profiles" --seconds 8
```

自动搜索接收器：

```powershell
.\build-examples-msvc\Release\partner_workflow.exe auto `
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

以上 Windows 命令从随包 JSON 骨架启动。已有兼容的个人档案时，加 `--profile 文件路径` 恢复校准。关闭窗口或按 Ctrl+C 退出。

### 4.9 Windows 完整 Qt 示例

已编译的 Windows x64 示例可直接打开 [qt_skeleton_viewer.exe](qt_skeleton_viewer/bin/qt_skeleton_viewer.exe)。该目录已随附 Qt 和 SDK 运行库。

Qt 左侧显示 SDK 线框，右侧显示 Unity FBX 三维手部模型。三维视图使用 SDK 的 Windows 组件 `HexQtFbxView.dll`，模型资源随组件内置。运行电脑需要支持 OpenGL 3.3 的驱动。

已有 Qt 项目时，可从 [Qt 三维视图接入说明](../接口文档/QT_FBX_INTEGRATION.zh-CN.md) 和 [最小接入示例](qt_fbx_embed/main.cpp) 开始。Windows 示例使用 Qt 6.5.3 MSVC x64 Release。

使用独立的 Windows 构建目录，不要复用 Linux 的 CMake 缓存。

```powershell
cmake -S .\examples\qt_skeleton_viewer -B .\build-qt65-win `
    -G "Visual Studio 17 2022" -A x64 `
    "-DCMAKE_PREFIX_PATH=$SdkQt"
if ($LASTEXITCODE -ne 0) { throw "Qt 配置失败" }

cmake --build .\build-qt65-win --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Qt 编译失败" }

.\build-qt65-win\Release\qt_skeleton_viewer.exe
```

不带参数启动后，可以先选连接模式，再选串口并点击“连接手套”。**无线**选择接收器串口。**有线**分别选择左手和右手的数据串口，两项必须不同。若设备同时列出 CH342 A/B 两个接口，请按实际业务通道选择。请核对每手新帧及时间戳，以选定实际传输数据的通道。

也可以指定串口并立即连接。

```powershell
.\build-qt65-win\Release\qt_skeleton_viewer.exe $SdkPort --no-profile
.\build-qt65-win\Release\qt_skeleton_viewer.exe --wired COM12 COM14 --no-profile
```

`--wired` 后按左手、右手的顺序传入实际端口。使用 `--no-profile` 从默认 M11 骨架开始，关闭档案读写，快速 R 和七步结果保留在当前会话。使用 `--profile "你的档案.hexprofile"` 可显式加载档案。省略档案参数时，无线按接收器串口自动匹配，有线按有序的左右串口组合匹配。

Windows Qt 的七步流程先采集平放、伸直、握拳，再为食、中、无名、小指分别采集指腹捏与指尖掐。两姿态间可以松开。留意每手的端点采样提示。“重试当前对指”保留已采端点。步骤暂停后，“重新采集当前手指两姿态”重置当前指的两个端点，其他指保留。Windows Qt 的对指吸附默认关闭。在完整示例窗口按住 `Ctrl` 和 `X`，再按一下 `F` 切换双手吸附。校准期间暂停切换。切换结果见“事件记录”，重新连接时沿用本次选择。Linux Qt 使用单接收器、匹配档案和旧版连续运动七步。

CMake 会复制 SDK、三维控件 DLL 和随包 VC 运行库。在本节 PowerShell 中可从已设置的路径加载 Qt。若要在文件管理器启动新构建的程序，参照 [Qt 三维视图接入说明](../接口文档/QT_FBX_INTEGRATION.zh-CN.md)复制 Qt DLL 与平台插件。

### 4.10 Windows Qt FBX 最小接入示例

`qt_fbx_embed` 演示如何在 Qt Widgets 应用中嵌入随 SDK 提供的三维手部控件。使用 **Qt 6.5.3 MSVC x64 Release** 和支持 OpenGL 3.3 的显卡驱动。控件自带左右手模型和待机姿态。从 SDK 根目录运行第 4.1 节的准备命令后，执行以下命令。

```powershell
cmake -S .\examples\qt_fbx_embed -B .\build-qt-fbx `
    -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=$SdkQt"
if ($LASTEXITCODE -ne 0) { throw "Qt FBX 配置失败" }

cmake --build .\build-qt-fbx --config Release --target qt_fbx_embed
if ($LASTEXITCODE -ne 0) { throw "Qt FBX 编译失败" }

.\build-qt-fbx\Release\qt_fbx_embed.exe
```

无参数时显示双手待机模型。要接入实机，运行下面任一命令，并换成实际数据口。双有线参数依次为左手和右手。手套开始输出后，将手掌平放、手指自然伸直，在窗口内按 `R` 做两秒快速校准。

```powershell
.\build-qt-fbx\Release\qt_fbx_embed.exe COM3
.\build-qt-fbx\Release\qt_fbx_embed.exe --wired COM12 COM14
```

CMake 会将 `HexQtFbxView.dll`、`HexaCercleGlove.dll` 及随包 VC 运行库复制到 EXE 目录。需要直接从文件管理器启动时，按三维控件接入与部署一节把同一 Qt 套件的运行库和平台插件放到 EXE 旁。

[三维控件接入与部署](../接口文档/QT_FBX_INTEGRATION.zh-CN.md) · [最小示例源码](qt_fbx_embed/main.cpp)

## 5. 参数、输出与结果判断

### 5.1 常用参数

| 参数 | 适用示例 | 含义 |
| --- | --- | --- |
| `--profile 文件` | 快速入门、统一快照、MuJoCo、Qt 完整查看器 | 加载已有校准档案 |
| `--lib 文件` | Python、MuJoCo | 指定原生 SDK 动态库 |
| `--csv 文件` | 快速入门、统一快照、Qt 完整查看器 | 导出角度 CSV |
| `--jsonl 文件` | 快速入门、统一快照、Qt 完整查看器 | 导出角度 JSON Lines |
| `--jsonl -` | 上述支持 JSON Lines 的示例 | 将逐帧 JSON 写到标准输出 |
| `--no-profile` | Qt 完整查看器 | 从 Unity 待机姿态启动，本次会话停用档案读写 |
| `--wired 左串口 右串口` | Windows Qt 上位机、Qt FBX 最小示例 | 使用独立的左右手有线端口并立即连接 |
| `--r` | 控制台快速入门、统一快照、连接工作流、MuJoCo | 加载档案后仍执行普通 R |
| `--hand left/right/both` | 连接工作流、MuJoCo | 选择 R 或显示的手 |
| `--seconds N` | 连接工作流 | 运行约 N 秒 |
| `--ticks N` | Python 统一快照 | 执行 N 轮更新 |

需要重新普通 R 时，在对应命令末尾加 `--r`；工作流可加 `--r --hand both`。R 期间保持手掌平直稳定。Qt 的“R 快速校准（2 秒）”会重新采集手掌及手指的佩戴零位。传感器 R 与几何角度零位不同，重新 R 后应重新建立角度零位。普通 R 不能替代匹配佩戴状态的完整校准。

### 5.2 如何判断测试结果

- 无线接收器回复后显示六位 ID（历史示例为 `114205`）。Windows 双有线连接分别显示左右数据口并检查每手新帧。
- 每手骨骼 `tracked_mask=001fffff`，或十进制 `2097151`，表示 21 节点都在跟踪。仅 `valid_mask` 有值不代表实时跟踪。
- 快照 `online/tracked/angles` 为 `3` 时表示左右手对应位均有效；输出应同时检查新帧有效位。
- 工作流 `phase=4`、`streaming=3` 表示双手数据流；`profile=1` 表示已加载档案。
- CSV/JSON 应有左右手记录、时间戳、帧号和 15 个角度字段，单位为度。已有文件可能被同名运行覆盖。
- MuJoCo 日志应出现 `online_mask=3`、`left:tracked right:tracked`；灰色模型不能作为实时跟踪证据。
- Qt 完整查看器 未归零前不输出新角度；骨骼正常不代表角度零位已完成。
- 界面显示的观测帧率、双手到达间隔不等于硬件采样率、端到端延迟或硬件同步证明。

### 5.3 常见问题

| 现象 | 检查方式 |
| --- | --- |
| 串口打开失败 | 核对实际端口、关闭占用程序、检查驱动；Linux 确认当前用户有串口权限 |
| Linux 找不到 `.so` | 使用上述配套目录；Python 传绝对 `--lib` 路径，C++ 保留 rpath 或配置 `LD_LIBRARY_PATH` |
| Windows 找不到 DLL / 错误 126 | 确认主 DLL 和依赖齐全，SDK 目录在搜索路径，Python 和 DLL 均为 64 位 |
| Windows 架构错误 / 错误 193 | 检查 Python、MSVC 目标平台、Qt、SDK 是否同为 x64 |
| Qt 找不到平台插件 | 使用对应 Qt 套件运行环境，或执行 `windeployqt`；不要混用 Qt5/Qt6 插件 |
| Qt 实际使用了旧版本 | 检查 CMake 的 `Qt6_DIR` 与运行库路径；切换版本时新建构建目录 |
| MuJoCo 无法创建图形窗口 | 先执行 `--check-gl`；Linux 在本地图形终端检查显示会话和 OpenGL 环境 |
| 工作流 `profile=0` | 按实际选中的串口路径生成档案键，不要把 COM 端口或 Linux 别名当成同一个键 |
| 没有角度导出 | 检查是否有新鲜且完整的跟踪帧、R 是否就绪、角度零位是否建立 |

详细 API 与生命周期见 [接口文档](../接口文档/README.zh-CN.md)，示例原理见 [SDK 使用说明](SDK_USAGE_PYTHON_CPP_QT.zh-CN.md)，MuJoCo 详细说明见 [MuJoCo README](mujoco_sdk_skeleton/README.zh-CN.md)。
