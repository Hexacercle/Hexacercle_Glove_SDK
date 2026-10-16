# 外部接入库文件

本目录给外部开发者使用。`include/hex_glove/` 保留当前对外接入文档与示例编译所需的 **21 个头文件**，包含必要的递归类型声明；对外支持范围见 `../EXTERNAL_API.zh-CN.md` 与 `../PARTNER_INTEGRATION.zh-CN.md`，包括连接、基础设备信息、校准文件、R、21 节点骨骼及关节角、状态诊断、ABI 检查、统一快照，以及接收器搜索/重连、标定与配置自动管理。`hex_hardware_info_api.h` 声明接收器设备 ID 查询；Qt 示例还使用 `hex_pipeline_api.h` 读取和切换腕部 X/Y 同步。`hex_partner_api.h` 声明上位机对接流程，`hex_external_api.h` 声明快照及错误信息接口，`mocap_abi.h` 声明 ABI 与能力结构；CSV/JSON/实时流由接入应用消费 SDK 帧后写出。

| 路径 | 用途 |
| --- | --- |
| `include/hex_glove/` | 精简声明及其必要类型；Python `ctypes` 不需要头文件。 |
| `windows-x64/HexaCercleGlove.dll` | 当前源码构建的 Windows x64 主动态库。 |
| `windows-x64/libHexaCercleGlove.dll.a` | 配套 MinGW/GCC 链接库；MSVC 不能直接使用。 |
| `windows-x64/libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll` | Windows 运行所需的 MinGW 依赖。 |
| `linux-x64/libHexaCercleGlove.so` | 2026-10-15 从当前源码重新构建的 Linux x64 动态库；外部支持范围仍以本目录 API 文档为准。 |

动态库自身仍可能导出未列在精简头文件中的内部函数；精简头文件和外部文档定义当前对外支持范围，并不限制二进制导出。Windows 示例需使用本目录头文件和配套链接库编译链接；本轮验证结果以根目录 README 为准。生产发布前应在目标 Linux 环境验证 `.so` 的运行兼容性和真实手套通信。



