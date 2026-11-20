# 接入库与头文件

| 目标 | 头文件搜索目录 | 链接与部署 |
| --- | --- | --- |
| Windows x64 · MSVC | `include` | 链接 `windows-x64/HexaCercleGlove.lib`，部署同目录的 `HexaCercleGlove.dll`、`msvcp140*.dll` 与 `vcruntime140*.dll`。 |
| Linux x64 · 原 10.16 版本 | `linux-x64/include` | 链接 `linux-x64/libHexaCercleGlove.so`。该库原构建环境为 Ubuntu 22.04、GCC 11.4。 |

Windows 库使用 Visual Studio 2022、MSVC v143、x64 Release、动态 CRT（`/MD`）构建。`windows-x64-MSVC` 和 `windows-x64` 提供相同的 DLL、导入库与运行依赖。升级应用时，请同步替换头文件、导入库及应用实际加载的 DLL。

| Windows 入口头文件 | 用途 |
| --- | --- |
| `hex_glove/hex_partner_api.h` | 无线接收器或左右两个有线串口的连接、断线重试、状态及校准档案管理 |
| `hex_glove/hex_external_api.h` | `hex_external_create`、`hex_external_profile_load`、`hex_external_update` 和双手一致快照 |
| `hex_glove/hex_glove_api.h` | 直接打开无线或有线数据口，检查串口打开错误和每只手的数据 |
| `hex_glove/hex_mocap_api.h` | 骨骼、快速 R、两姿态七步校准、按手读取采集进度、双手对指吸附开关 |
| `hex_glove/hex_hardware_info_api.h` | 查询无线接收器设备 ID |

Linux 使用自身目录内的声明与库；有线双串口和新版端点校准由本包 Windows 库提供。Python 可通过 64 位解释器和 `ctypes` 加载对应平台的动态库。[API 概览](../EXTERNAL_API.zh-CN.md) · [上位机工作流](../PARTNER_INTEGRATION.zh-CN.md) · [示例](../../examples/README.md)
