# HexaCercle 手套 SDK｜对外接入文档

> **交付范围**：Windows x64、Linux x64 · C ABI · C++17 / Qt / Python 接入  
> **文档版本**：2026-10-15 · 以本包 `library/include/hex_glove/` 与同目录库文件为准

## 从哪里开始

| 你要做什么 | 阅读文档 | 得到什么 |
| --- | --- | --- |
| 了解可开放能力 | [API 概览](EXTERNAL_API.zh-CN.md) | 连接、设备 ID、状态、校准、骨骼与关节角的功能边界。 |
| 首次运行 SDK | [接入与调用指南](EXTERNAL_INTEGRATION.zh-CN.md) | 环境准备、句柄创建、连接、读帧、校准、断开的顺序。 |
| 查函数签名和参数 | [函数参数手册](EXTERNAL_API_PARAMETERS.zh-CN.md) | 输入、输出、返回值、有效性条件、关键调用示例与错误处理。 |
| 做上位机自动搜索与重连 | [工作流接口指南](PARTNER_INTEGRATION.zh-CN.md) | `hex_partner_*` 状态机、事件、标定和档案管理。 |
| 核对交付条件 | [Windows／Linux C++ SDK 交付状态](WINDOWS_CPP_SDK_DELIVERY_STATUS.zh-CN.md) | 对接需求、支持状态、构建环境与限制。 |
| 直接看代码 | [Python／C++／Qt 示例](../examples/README.md) | 可运行的串口、骨骼、角度和设备 ID 读取示例。 |

## 五步接入

1. 从 [library](library/README.zh-CN.md) 选择与运行平台一致的头文件和动态库；Windows 使用随包 MinGW x64 导入库，Linux 使用 `.so`。
2. 创建 `glove`、`mocap` 和可选的 `external`；如有 `.hexprofile`，在开始实时求解前加载。
3. 打开串口。SDK 自动请求接收器六位设备 ID；轮询 `hex_glove_get_device_id`，收到有效回复后才显示或保存。
4. 只在本轮新帧、骨骼跟踪和角度有效位满足要求时消费数据。R 校准与伸直中立位的几何角度校准是两步。
5. 按 `external → mocap → glove` 的依赖顺序销毁；使用 partner 时先断开并销毁 partner。

## 接口选择

| 接入方式 | 推荐场景 | 每轮推进函数 | 数据读取 |
| --- | --- | --- | --- |
| `hex_partner_*` | 自动搜串口、重连、操作结果与事件 | `hex_partner_tick` | `hex_partner_get_status`；可配合 `hex_external_get_frame`。 |
| `hex_external_*` | 已知端口，需要左右手同一宿主更新轮次的快照 | `hex_external_update` | `hex_external_get_frame`。 |
| `hex_glove_*` + `hex_mocap_*` | 需要自主控制连接、骨架或求解过程 | `hex_mocap_update` | 设备快照、每手 21 节点和角度。 |

同一轮只选择**一种推进函数**。`HexExternalFrame` 保证数据来自同一次宿主更新，但不保证左右设备在硬件上同步采样。接收器设备 ID 为单个六位标识；左右手套各自的两字符短码不能当作独立唯一序列号。现有校准档案仍按端口和左右手槽位绑定。

## 交付目录

```text
接口文档/
├─ README.zh-CN.md                 文档入口
├─ EXTERNAL_API.zh-CN.md           API 概览与数据语义
├─ EXTERNAL_API_PARAMETERS.zh-CN.md 函数参数与关键接口示例
├─ EXTERNAL_INTEGRATION.zh-CN.md   接入顺序
├─ PARTNER_INTEGRATION.zh-CN.md    自动搜索与重连工作流
└─ library/
   ├─ include/hex_glove/          对外头文件
   ├─ windows-x64/                DLL、导入库和依赖
   └─ linux-x64/                  SO
```

**最先检查的三个结果**：`hex_glove_get_device_id` 返回 `1`；`fresh_hand_mask` 对应手位为 `1`；`pose.tracked_mask` 有目标节点。串口已打开、函数返回成功或模型节点存在，都不能单独证明当前有实时手势数据。
