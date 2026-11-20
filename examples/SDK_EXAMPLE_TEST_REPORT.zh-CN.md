# SDK 示例复测与接收器 ID 查询记录

日期：2026-10-16（10.16 原包历史记录）

> 下文记录 10.16 的 MinGW、Linux 与实机反馈。11.20-MSVC 的 Windows 库、Qt 和示例验证范围见[本版发布说明](../README.md)。

## 结论与证据范围

| 项目 | 结果 | 依据 |
| --- | --- | --- |
| 所有示例启动和正常退出 | 用户已确认 | 用户在目标设备上的运行反馈；本次未重复执行全套硬件测试 |
| MuJoCo、Qt 双手跟踪 | 用户已确认 | 用户在目标设备上的运行反馈；本次未重复执行图形窗口测试 |
| 三个命令行示例输出骨骼和角度 | 用户已确认 | 用户运行日志反馈 |
| 三个命令行示例读取六位接收器 ID | 待硬件复测 | 原示例在启动后立即发信道查询；本次已调整查询顺序，尚无真实接收器的新日志 |
| 两个 C++ 示例编译链接 | 通过 | 使用发布包的 Windows x64 头文件及 MinGW 导入库，以 C++17 编译 |
| Python 快照示例语法 | 通过 | 对修改后的 `main.py` 执行 Python `compile` 检查 |
| SDK 接收器 ID 协议解析 | 通过 | `protocol_selftest.exe` 通过；包含示例回复 `114205` 的解析检查 |

## 修改的三个命令行示例

- `sdk_quickstart/main.cpp`
- `external_snapshot/main.cpp`
- `external_snapshot/main.py`

SDK 打开串口后自动查询接收器 ID。三个示例通过 `hex_glove_get_device_id` 轮询查询结果，必要时调用 `hex_glove_request_device_id` 重试。示例在收到 ID 或等待超时后继续查询信道，收到有效 ID 后会输出六位 `receiver_device_id`。查询顺序调整后的效果仍需在目标设备上复测。

硬件复测时，请在目标机上分别运行三个示例，确认日志出现六位 `receiver_device_id`，并核对信道、骨骼和角度。若仍显示 `waiting`，请记录所用串口、连接状态和 SDK 日志。

## 自动搜索与校准档案绑定

`hex_partner_*` 的自动搜索/重连会使用本轮实际选中的接收器端口字符串创建档案名：将该字符串的 UTF-8 字节转成十六进制，形成 `receiver-<十六进制>.hexprofile`。例如 `COM3` 对应 `receiver-434f4d33.hexprofile`。一个档案包含同一接收器下的左右手校准数据。可用 `hex_partner_profile_path` 查询当前实际路径。

此绑定依据端口名或端口别名，**尚未依据六位物理接收器 ID**。自动搜索在不同运行中若选中不同端口号或 `/dev/serial/by-id/...` 别名，会映射到不同档案；原档案不会自动迁移。部署时建议固定 `preferred_port_utf8` 或使用稳定的端口别名，并核对界面/日志中的档案路径。三个命令行示例仅在显式传入 `--profile` 时加载指定文件，不使用 partner 的自动档案命名。
