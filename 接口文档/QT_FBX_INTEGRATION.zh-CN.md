# Qt 三维手部视图接入

更新日期 2026-11-25 · 对应 11.20-MSVC

`HexQtFbxView` 是适用于 11.20-MSVC SDK 的 Qt Widgets 组件。它将 SDK 每手 21 节点的姿态显示为与 Unity `kulou_shou_New1.fbx` 骨架配套的三维手部网格。接入现有 Qt 应用前，可以先运行最小示例查看左右手的待机姿态。

## 选择示例

| 场景 | 入口 | 校准与显示 |
| --- | --- | --- |
| 集成或快速试用三维视图 | [`examples/qt_fbx_embed`](../examples/qt_fbx_embed/main.cpp) | 无参数查看待机姿态。用命令行指定串口，等数据到达后按键盘 `R` 快速校准。窗口显示两只三维手及状态提示。 |
| 使用完整包中的 Windows 查看器 | `examples/qt_skeleton_viewer/bin/qt_skeleton_viewer.exe` | 用左栏选择连接方式和串口，点击「R 快速校准（2 秒）」。窗口并排显示线框和三维网格，并提供标定进度、实时角度、档案和事件记录。 |
| 维护历史线框工程 | 公开包保留的 `examples/qt_skeleton_viewer`，以及完整包中的 Linux Qt 示例 | 适用于现有线框流程。Linux x64 继续使用随包的 Linux 库和单无线接收器。 |

## 编译并运行最小示例

在解压后同时包含 `examples` 和 `接口文档` 的 SDK 根目录运行。已有同版本 11.20-MSVC 的项目可按下表目录合并 Qt 接入组件与示例，继续使用现有 SDK 运行库。

| 相对路径 | 用途 |
| --- | --- |
| `接口文档/library/include/hex_glove/` | 11.20-MSVC 公开头文件及 Qt 视图入口 `hex_qt_fbx_view.h` |
| `接口文档/library/windows-x64/HexaCercleGlove.dll`、`HexaCercleGlove.lib` | 原版 11.20-MSVC SDK 及导入库 |
| `接口文档/library/windows-x64/HexQtFbxView.dll`、`HexQtFbxView.lib` | Qt 视图与配套模型数据及视图导入库 |
| `接口文档/library/windows-x64/` 中的 `msvcp140*.dll` 和 `vcruntime140*.dll` | 随应用部署的 x64 Visual C++ 运行库 |
| `examples/qt_fbx_embed` | 最小 Qt Widgets 示例及 CMake 工程 |

三维组件需要 **Windows x64、Visual Studio 2022 C++ 工具链、Qt 6.5.3 MSVC x64 Release** 和支持 **OpenGL 3.3** 的显卡驱动。编译与运行使用同一套 Qt Release 库。模型网格、默认模型映射和待机姿态随 `HexQtFbxView.dll` 提供。

在解压后的包根目录打开 PowerShell，将 `$Qt` 改为本机 Qt 套件的实际安装目录，运行以下命令。

```powershell
$Qt = 'C:\Qt\6.5.3\msvc2019_64'
$Qt = (Resolve-Path -LiteralPath $Qt).Path
cmake -S .\examples\qt_fbx_embed -B .\build-qt-fbx-embed `
    -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_PREFIX_PATH=$Qt"
if ($LASTEXITCODE) { throw 'Qt 示例配置失败' }
cmake --build .\build-qt-fbx-embed --config Release
if ($LASTEXITCODE) { throw 'Qt 示例编译失败' }
$env:Path = "$Qt\bin;$env:Path"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = "$Qt\plugins\platforms"
.\build-qt-fbx-embed\Release\qt_fbx_embed.exe
```

无参数时，窗口显示左右手待机姿态。需要接入实机时，以下两条命令任选一条，并将示例端口改为实际 COM 口。

```powershell
# 无线接收器
.\build-qt-fbx-embed\Release\qt_fbx_embed.exe COM3

# 双有线数据口，左手在前，右手在后
.\build-qt-fbx-embed\Release\qt_fbx_embed.exe --wired COM12 COM14
```

最小示例显示「平掌伸直，按 R 快速校准」后，平放手掌、自然伸直手指，在窗口内按 `R` 并保持约两秒。示例按当时正在输出的手发起校准。要同时校准双手，等左右手都在持续输出后再按 `R`。「快速 R 完成」后，有效的实时姿态驱动模型。断流时恢复待机姿态。M7/M11 输入变化时，按窗口提示重新进行 R。最小示例每次从默认模型开始，校准结果保留在当前会话。个人档案管理和七步校准可使用相应的查看器。

## 操作完整 Windows 查看器

1. 在完整包中启动 `examples/qt_skeleton_viewer/bin/qt_skeleton_viewer.exe`。左侧「设备连接」选择「无线（单接收器）」或「有线（双串口）」。有线模式按左、右手各选一个不同的实际数据口，再点击「连接手套」。
2. 顶部状态显示连接阶段、「输出」和「新帧」的手别、上位机观测的新帧率和电量。先等所选手输出数据，再平放手掌、自然伸直手指，点击「R 快速校准（2 秒）」。通过「标定进度」查看采集结果。
3. 校准完成且收到跟踪帧后，「SDK 骨骼线框」和「FBX 手部模型」并排显示对应手势。需要看关节角时，手指伸直并点击「角度归零（手指伸直）」。需要采集完整的标准 11 通道两姿态七步校准时，使用「开始双手七步校准」和「标定进度」中的提示。

**对指吸附快捷键属于完整 Windows 查看器。** 启动时吸附关闭。在查看器窗口中让 `Ctrl`、`X`、`F` 同时处于按下状态，按键顺序不限。常用操作是按住 `Ctrl` 和 `X`，再轻按 `F`。三键组合每次只切换一次，长按不会连切。松开任意一键后可以再次触发。开关状态见「事件记录」，本次运行内重新连接会沿用当前选择。退出串口输入框的编辑状态后使用该快捷键，在校准结束后再切换。

完整版也可用 `--no-profile` 做一次不读写校准档案的验证。加载正式档案时，请使用与当前手套、佩戴状态和 Unity 骨架匹配的档案。

## 嵌入 Qt 应用

直接嵌入现有应用时，添加 `接口文档/library/include` 头文件目录，链接 `HexQtFbxView.lib` 和 `Qt6::Widgets`。宿主直接调用手套 SDK 时也链接 `HexaCercleGlove.lib`。将随包提供的 `HexQtFbxView.dll`、`HexaCercleGlove.dll` 和 x64 Visual C++ 运行库放在应用 EXE 同目录。通过 Qt 套件或应用部署目录提供 `Qt6Core.dll`、`Qt6Gui.dll`、`Qt6Widgets.dll`、`Qt6OpenGL.dll`、`Qt6OpenGLWidgets.dll` 以及 `platforms/qwindows.dll`。

若要直接从文件管理器启动最小示例，可在前面已设置 `$Qt` 的 PowerShell 中执行以下命令，将这些 Qt 依赖复制到同一目录。完整查看器自行构建时，将 `$appDir` 换成其 EXE 目录。

~~~powershell
$appDir = '.\build-qt-fbx-embed\Release'
$qtDlls = @('Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6OpenGL.dll', 'Qt6OpenGLWidgets.dll')
foreach ($dll in $qtDlls) {
    Copy-Item -LiteralPath (Join-Path "$Qt\bin" $dll) -Destination $appDir
}
$platformDir = Join-Path $appDir 'platforms'
New-Item -ItemType Directory -Path $platformDir -Force | Out-Null
Copy-Item -LiteralPath "$Qt\plugins\platforms\qwindows.dll" -Destination $platformDir
~~~

宿主使用其他 Qt 模块或插件时，按宿主的依赖一并部署。发布前在目标电脑检查启动和三维绘制。

## 设置三维视图背景

使用支持背景色的新版 `HexQtFbxView.dll` 时，在 Qt GUI 线程设置 `hex_qt_fbx_create` 返回控件的 `QPalette::Window`。可以在第一次显示前设置，也可以在已经打开的窗口中换色。默认清屏色为 `#191D26`。

~~~cpp
#include <QColor>
#include <QPalette>
#include <QWidget>

// sdkFbxView 是 hex_qt_fbx_create 返回的 QWidget*。
QPalette colors = sdkFbxView->palette();
colors.setColor(QPalette::Window, QColor("#FF0000"));
sdkFbxView->setPalette(colors);
~~~

Qt 6.5.3 下，也可直接对该控件调用 `setStyleSheet("background-color: #FF0000;")`。如果宿主样式表同时设置背景，请统一颜色来源。

三维视口使用不透明背景。视口外的边框、留白和标题可在承载控件上设置样式，留白宽度由父布局的 `setContentsMargins` 控制。换色时请确认宿主 EXE 加载的是新版随包 DLL。

## 公开入口

包含 `hex_glove/hex_qt_fbx_view.h`。`hand` 为 `0` 时表示左手，为 `1` 时表示右手。`mask` 的 bit 0 和 bit 1 对应左、右手。

| 函数 | 用法 |
| --- | --- |
| `hex_qt_fbx_create(parent)` | 在 `QApplication` 创建后调用。返回 `QWidget*`。有父控件时由父控件销毁，无父控件时由宿主负责销毁。返回 `nullptr` 表示创建失败。 |
| `hex_qt_fbx_set_pose(view, hand, pose)` | 提交指定手的姿态。返回成功表示控件接受本次提交。视图以掌面自动定向。传 `nullptr` 将该手恢复至待机姿态。 |
| `hex_qt_fbx_set_pose_with_basis(view, hand, pose, up, right)` | 需要与已有骨骼视图共享朝向时，同时传入同一世界坐标系下的上方和右方向量。 |
| `hex_qt_fbx_get_idle_pose(hand, out)` | 将配套待机姿态写入调用方提供的 `HexMocapDisplayPose`。调用前设置 `out->struct_size = sizeof(*out)`。可用于初始化并排的线框显示。 |
| `hex_qt_fbx_configure_default_model(mocap)` | 在新建的 `HexMocapHandle` 上安装与内置网格配套的默认 Unity 模型，双手初始为 11 通道输入映射。 |
| `hex_qt_fbx_set_input_layout(mocap, hand, layout)` | 为指定手安装 `7` 或 `11` 通道的输入映射。 |
| `hex_qt_fbx_get_input_layout(mocap, hand)` | 返回驱动观察到的输入通道数 `7` 或 `11`。布局未识别或读取失败时返回 `0`。 |
| `hex_qt_fbx_start_quick_r(mocap, mask)` | 对所选手发起两秒佩戴零位校准。`1`、`2`、`3` 分别选择左手、右手和双手。请求受理后，通过 SDK 状态接口检查进度和结果。 |

除创建和读取布局外，上表函数以 `1` 表示调用成功，以 `0` 表示失败。创建控件、向控件提交姿态和销毁控件都在 Qt GUI 线程进行。如果 SDK 在工作线程采集，用队列将完整的姿态副本交给 GUI 线程。

## 接入现有更新循环

1. 创建 SDK 手套和 mocap 会话。新会话调用一次 `hex_qt_fbx_configure_default_model`，再连接设备。已有配套 Unity 骨架的个人档案时，改用 `hex_mocap_profile_load`，沿用档案的模型和输入映射。
2. 默认模型接到数据后，用 `hex_qt_fbx_get_input_layout` 查询左右手的输入布局。观察值与已安装的布局不同时，在校准前对该手调用 `hex_qt_fbx_set_input_layout`。左右手的布局可以独立设置。切换后，继续推进 SDK 并等待该手的新帧。
3. 创建视图并加入 Qt 布局。需要重新校准时，确认所选手已持续输出数据，手掌平放、手指自然伸直，再调用 `hex_qt_fbx_start_quick_r`。保持姿势并持续推进 SDK 更新，使用 `hex_mocap_get_r_capture_info` 确认 `HEX_MOCAP_R_COMPLETE` 后开始显示实时姿态。
4. 沿用宿主的一条 SDK 更新链。使用 `hex_partner_tick` 管理连接时，每轮推进一次后读取姿态。直接接入的应用按既有流程调用 `hex_mocap_update` 或 `hex_external_update`。每只手独立检查新帧和跟踪状态。校准前、断流或断开时，可将对应手恢复到待机姿态。

对内置模型，直接传入配套 Unity 骨架的 `HexMocapDisplayPose`。保留 SDK 的节点顺序、以米为单位的世界矩阵平移和局部位置，并设置 `pose.struct_size = sizeof(pose)`。组件按内置网格和节点映射完成显示，宿主可以原样传递 SDK 姿态。使用其他 FBX、修改骨架层级或换用不同档案时，需按相应模型管理映射。

显式加载指定档案后，按宿主的档案策略设置 `HexPartnerConfig.auto_load_profile` 和 `auto_save_profile`。需要在连接时继续使用当前档案时，将自动加载设为 `0`。

下面的代码放在宿主的 GUI 线程。已有 `QApplication` 后调用 `attachFbxPanel`。在 partner 定时器中调用 `updateFbxPanel`，由该函数推进本轮 SDK。`calibratedMask` 记录宿主已确认可用于当前佩戴状态的手，例如本次 R 成功后对应的手。串口生命周期见[上位机接入指南](PARTNER_INTEGRATION.zh-CN.md)。

```cpp
#include <QLayout>

#include "hex_glove/hex_mocap_api.h"
#include "hex_glove/hex_partner_api.h"
#include "hex_glove/hex_qt_fbx_view.h"

QWidget* attachFbxPanel(QWidget* parent, QLayout* layout) {
    QWidget* view = hex_qt_fbx_create(parent);
    if (view) layout->addWidget(view);
    return view;
}

void updateFbxPanel(QWidget* view, HexPartnerHandle partner, HexMocapHandle mocap,
                    int64_t monotonicMs, uint32_t calibratedMask) {
    if (!view) return;
    HexPartnerStatus status{};
    status.struct_size = sizeof(status);
    const bool updated = hex_partner_tick(partner, monotonicMs)
        && hex_partner_get_status(partner, &status);
    for (int hand = 0; hand < 2; ++hand) {
        const uint32_t bit = 1u << hand;
        if (!updated || !(status.streaming_hand_mask & calibratedMask & bit)) {
            hex_qt_fbx_set_pose(view, hand, nullptr);
            continue;
        }
        if (!(status.fresh_hand_mask & bit)) continue;

        HexMocapDisplayPose pose{};
        pose.struct_size = sizeof(pose);
        if (!hex_mocap_get_display_pose(mocap, hand, &pose) || !(pose.tracked_mask & 1u)
            || !hex_qt_fbx_set_pose(view, hand, &pose)) {
            hex_qt_fbx_set_pose(view, hand, nullptr);
        }
    }
}
```

## 常见问题

| 现象 | 检查 |
| --- | --- |
| 程序提示 DLL 缺失或加载错误 | 核对应用、SDK、组件和 Qt 均为 x64，Qt 版本为 6.5.3 MSVC Release。确认两个 SDK DLL 和随包 VC 运行库已复制到应用目录。 |
| Qt 提示无法加载 Windows 平台插件 | 在启动程序的同一终端核对 `PATH` 和 `QT_QPA_PLATFORM_PLUGIN_PATH`，确认 `qwindows.dll` 来自相同 Qt 套件。 |
| 线框有显示，三维区域报错 | 检查显卡驱动的 OpenGL 3.3 支持，以及 `Qt6OpenGL.dll`、`Qt6OpenGLWidgets.dll` 是否可加载。 |
| 连接成功后仍是待机姿态 | 按左右手检查实际串口、持续输出、新帧和跟踪位。完成所选手的 R，或加载能够用于当前手的配套档案。布局发生切换时，等待新帧再重新校准。 |
| 模型朝向或骨段位置异常 | 核对档案和输入布局属于配套 Unity 模型，检查 `valid_mask`、`tracked_mask`、21 节点顺序和 `world_matrix`。共享线框坐标轴时，给 `hex_qt_fbx_set_pose_with_basis` 同时传入同一世界坐标系下的两个方向向量。 |

## 迁移原 Qt 骨骼查看器

如果现有项目源自 11.20-MSVC 的 `examples/qt_skeleton_viewer`，保留原有串口、校准、角度和线框逻辑。构建时链接 `HexQtFbxView.lib`。使用 `hex_qt_fbx_create` 创建三维面板，将原先向三维面板提交姿态和清除手的调用替换为 `hex_qt_fbx_set_pose` 或 `hex_qt_fbx_set_pose_with_basis`。默认模型和快速 R 可以使用上表中的公开调用。随包最小示例演示这条接入路径。
