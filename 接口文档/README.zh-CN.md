# HexaCercle 手套 SDK｜对外接入文档

> **交付范围**：Windows x64、Linux x64 · C ABI · C++17 / Qt / Python 接入  
本说明对应 **11.20-MSVC**。Windows x64 使用本包 `library/include` 与配套 MSVC DLL、导入库，支持单无线接收器、双有线串口及新版七步对指校准。Linux x64 沿用 10.16 的单接收器库，编译时使用 `library/linux-x64/include`。

## 从哪里开始

| 你要做什么 | 阅读文档 | 得到什么 |
| --- | --- | --- |
| 了解可开放能力 | [API 概览](EXTERNAL_API.zh-CN.md) | 连接、设备 ID、状态、校准、骨骼与关节角的功能边界。 |
| 首次运行 SDK | [接入与调用指南](EXTERNAL_INTEGRATION.zh-CN.md) | 环境准备、句柄创建、连接、读帧、校准、断开的顺序。 |
| 查函数签名和参数 | [函数参数手册](EXTERNAL_API_PARAMETERS.zh-CN.md) | 输入、输出、返回值、有效性条件、关键调用示例与错误处理。 |
| 做上位机自动搜索与重连 | [工作流接口指南](PARTNER_INTEGRATION.zh-CN.md) | `hex_partner_*` 状态机、事件、标定和档案管理。 |
| 嵌入配套的 Unity 三维手部模型 | [Qt 三维手部视图接入](QT_FBX_INTEGRATION.zh-CN.md) | Windows 的 `HexQtFbxView` 组件、`kulou_shou_New1.fbx` 配套网格及最小示例。 |
| 接入自有 FBX 骨架 | [11.20 Qt 三维骨架说明](11.20说明文档.md) | SDK 的 21 节点、坐标和骨长对照、绑定及网格验收。 |
| 核对交付条件 | [库与头文件](library/README.zh-CN.md) | 随包库文件、头文件与构建环境。 |
| 直接看代码 | [Python／C++／Qt 示例](../examples/SDK_USAGE_PYTHON_CPP_QT.zh-CN.md) | 可运行的串口、骨骼、角度和设备 ID 读取示例。 |

## 五步接入

1. 从 [library](library/README.zh-CN.md) 选择与运行平台一致的头文件和动态库；Windows 使用随包 MSVC x64 `HexaCercleGlove.lib` 导入库；Linux `.so` 保留自 10.16 原包，本次未重编。
2. 创建 `glove`、`mocap` 和可选的 `external`；如有 `.hexprofile`，在开始实时求解前加载。
3. 无线模式打开一个接收器端口，回复后可读六位接收器 ID。Windows 有线模式按左手、右手指定两个不同数据口，通过 `hex_partner_create_wired` 创建工作流，并分别检查左右手新帧。
4. 只在本轮新帧、骨骼跟踪和角度有效位满足要求时消费数据。R 校准与伸直中立位的几何角度校准是两步。
5. 按 `external → mocap → glove` 的依赖顺序销毁；使用 partner 时先断开并销毁 partner。

## 接口选择

`hex_external_create`、`hex_external_profile_load` 和 `hex_external_update` 声明在 [hex_external_api.h](library/include/hex_glove/hex_external_api.h)，不在 `hex_mocap_api.h`。使用时包含 `#include "hex_glove/hex_external_api.h"`，并将本包 `接口文档/library/include` 设为头文件搜索根目录；链接配套的 MSVC `HexaCercleGlove.lib`，运行时使用同包 DLL。函数签名与配置步骤见[接入指南](EXTERNAL_INTEGRATION.zh-CN.md#external-接口头文件与-msvc-配置)，调用示例见 [external_snapshot/main.cpp](../examples/external_snapshot/main.cpp)。

| 接入方式 | 推荐场景 | 每轮推进函数 | 数据读取 |
| --- | --- | --- | --- |
| `hex_partner_*` | 自动搜串口、重连、操作结果与事件 | `hex_partner_tick` | `hex_partner_get_status`；可配合 `hex_external_get_frame`。 |
| `hex_external_*` | 已知端口，需要左右手同一宿主更新轮次的快照 | `hex_external_update` | `hex_external_get_frame`。 |
| `hex_glove_*` + `hex_mocap_*` | 需要自主控制连接、骨架或求解过程 | `hex_mocap_update` | 设备快照、每手 21 节点和角度。 |

同一轮只选择**一种推进函数**。`hex_external_update` 已在内部调用 `hex_mocap_update`，不要在同一轮重复调用这两个函数。若宿主已自行调用 `hex_mocap_update`，应使用 `hex_external_capture` 缓存结果，再用 `hex_external_get_frame` 读取。`HexExternalFrame` 保证数据来自同一次宿主更新，但不保证左右设备在硬件上同步采样。接收器设备 ID 为单个六位标识；左右手套各自的两字符短码不能当作独立唯一序列号。无线档案按接收器端口匹配，有线档案按有序的左右串口组合匹配。

## 交付目录

```text
接口文档/
├─ README.zh-CN.md                 文档入口
├─ EXTERNAL_API.zh-CN.md           API 概览与数据语义
├─ EXTERNAL_API_PARAMETERS.zh-CN.md 函数参数与关键接口示例
├─ EXTERNAL_INTEGRATION.zh-CN.md   接入顺序
├─ PARTNER_INTEGRATION.zh-CN.md    无线搜索、双有线与重连工作流
├─ QT_FBX_INTEGRATION.zh-CN.md     配套三维手部视图与最小示例
├─ 11.20说明文档.md               SDK 骨骼与自有 FBX 骨架接入
└─ library/
   ├─ include/hex_glove/          对外头文件
   ├─ windows-x64-MSVC/           原生 SDK、导入库和运行依赖
   ├─ windows-x64/                原生 SDK、运行依赖与 Qt 三维视图组件
   └─ linux-x64/                  SO
```

接入后检查每手的 `fresh_hand_mask` 和目标节点的 `pose.tracked_mask`，记录该手时间戳。无线模式可额外显示六位接收器 ID，双有线模式用左右数据口标识连接。
