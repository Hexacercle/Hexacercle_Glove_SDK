# Hexacercle Glove SDK

SDK, API documentation, and C++/Python examples for Hexacercle motion capture gloves.

Hexacercle 数据手套 SDK，提供设备接入接口、预编译动态库、中文开发文档及 C++ / Python 示例，帮助开发者读取双手骨骼与关节角数据，并接入自己的应用。

## 功能

- 接收器连接、设备信息查询及连接状态诊断。
- 双手每手 21 节点骨骼位置与旋转数据读取。
- 手指关节角读取及角度零位校准。
- 校准档案加载与管理。
- 接收器自动搜索与重连工作流。
- 通过示例导出带时间戳的 CSV / JSON Lines 数据。
- MuJoCo 双手骨骼可视化示例。

## 平台与接入方式

| 项目 | 说明 |
| --- | --- |
| Linux | 提供 x64 `.so` 动态库 |
| Windows | 提供 x64 DLL、MinGW 导入库及配套运行库 |
| C++ | 示例使用 C++17 |
| Python | 通过 `ctypes` 调用原生动态库 |
| MuJoCo | 可选可视化示例，需要 NumPy、MuJoCo 和 GLFW |

Windows 随包导入库适用于 MinGW，不能直接作为 MSVC 的 `.lib` 使用。应用程序、Python 与 SDK 动态库均需使用匹配的 64 位版本。

## 快速开始

1. 连接 Hexacercle 数据手套接收器，确认其串口名称。
2. 选择对应平台的 SDK 动态库。
3. 按接入指南准备环境，并使用匹配设备与佩戴状态的校准档案。
4. 运行 Python 或 C++ 快速入门示例。
5. 检查设备状态、跟踪状态及数据有效位，再使用骨骼和关节角数据。

完整编译命令、运行参数和数据导出方法见 [示例运行指南](examples/README.md)。

## 开发文档

- [文档入口](接口文档/README.zh-CN.md)
- [API 概览](接口文档/EXTERNAL_API.zh-CN.md)
- [首次接入与调用指南](接口文档/EXTERNAL_INTEGRATION.zh-CN.md)
- [函数参数手册](接口文档/EXTERNAL_API_PARAMETERS.zh-CN.md)
- [自动搜索与重连工作流](接口文档/PARTNER_INTEGRATION.zh-CN.md)
- [预编译库说明](接口文档/library/README.zh-CN.md)

## 示例

| 示例目录 | 内容 |
| --- | --- |
| `examples/python_quickstart/` | Python 快速入门 |
| `examples/sdk_quickstart/` | C++ 快速入门 |
| `examples/external_snapshot/` | Python / C++ 双手统一快照 |
| `examples/partner_workflow/` | C++ 设备连接工作流 |
| `examples/mujoco_sdk_skeleton/` | MuJoCo 骨骼可视化 |

## 当前验证范围

根据随包测试记录，部分示例已在 Ubuntu 22.04 x64 环境完成实机验证，详情见 [示例测试记录](examples/SDK_EXAMPLE_TEST_REPORT.zh-CN.md)。

随包校准档案用于特定设备与佩戴状态；使用其他设备时，应替换为自己的匹配档案。

## 许可说明

本仓库包含示例源码、接口头文件及 SDK 预编译库，未包含核心 SDK 的完整实现源码。

许可证及 SDK 二进制使用、分发条款将单独补充。

## 问题反馈

请通过本仓库的 Issues 提交问题，并附上操作系统、Python 或编译器版本、运行的示例及相关错误日志。
