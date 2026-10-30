# HexaCercle 手套 SDK｜自动连接与标定工作流

**10.27 工作流边界**：`hex_partner_*` 函数名、参数和对外状态结构未因内部 27 项新增而变化；自动档案仍按端口/左右槽位绑定，不因六位接收器 ID 自动迁移。新版档案和快速 R 行为差异见[10.16 → 10.27 接口变更说明](../SDK_CHANGELOG_10.16_TO_10.27.zh-CN.md)。

> **适用场景**：接入方上位机独立运行，SDK 管理搜索、重连、事件、双手标定及档案。  
> **主入口**：`hex_partner_api.h`；所有调用在同一应用线程串行执行。

[文档入口](README.zh-CN.md) · [API 概览](EXTERNAL_API.zh-CN.md) · [函数参数手册](EXTERNAL_API_PARAMETERS.zh-CN.md) · [基础接入](EXTERNAL_INTEGRATION.zh-CN.md)

## 本页目录

1. [连接、断开和重连](#1-连接断开和重连)
2. [设备标识与校准文件](#2-设备标识与校准文件)
3. [R 与完整标定](#3-r-与完整标定)
4. [骨骼与旧 UDP 字段](#4-双手骨骼与既有-udp-字段)
5. [运行环境和边界](#5-运行环境和已知边界)

Qt 示例的“七步手势校准”与本页 `hex_partner_calibration_begin(FULL)` 是两个不同入口。示例按 `_XGF` Unity 工程调用 `hex_mocap_seven_step_begin_original_ui(mocap,mask)`，一个入口同步选择当前有效的左右手，使用 `hex_mocap_seven_step_get_state` 查询自动推进的七步。界面不应再把旧的手动多步流程显示为 Unity 七步校准；完成后由接入程序保存 profile。七步取消用 `hex_mocap_seven_step_cancel`，当前对指重采用 `hex_mocap_contact_retry`。

## 串口错误与流控制补充

最近一次打开失败的 `status.port_error_stage/platform_error_code` 分别表示失败阶段与 Windows `GetLastError` 或 Linux `errno`。阶段 1 为端口名无效、2 为 OS 打开失败、3 为串口配置失败、4 为开始流命令写入失败。`last_error` 将常见系统码归为端口不存在、拒绝访问或占用、配置失败、启动命令失败；未知码仍为通用打开失败。自动探测多端口时记录的是最近一次尝试的端口和错误。Windows 拒绝访问无法仅凭错误码区分权限和占用。

接收器打开后调用 `hex_partner_stream_control(partner,0)` 暂停发流，状态进入 `HEX_PARTNER_STREAM_PAUSED`；传 `1` 重新请求发流，然后等待新帧进入 STREAMING。返回 1 和操作成功仅表示串口写入成功，硬件协议没有设备确认。标定过程中不要暂停；断线重连按协议默认重新发流。断线进入自动重试时 SDK 依次报告 DISCONNECTED 和 RETRYING 事件。每一轮连接失败会给本轮操作记录失败结果，下一次自动重试生成新的操作序号。Qt 示例提供暂停/恢复按钮。

本文针对“只启动接入方上位机”的接入方式。配套文件为 `library/include/hex_glove/hex_partner_api.h`、`hex_external_api.h`、Windows x64 `HexaCercleGlove.dll` 和 Linux x64 `libHexaCercleGlove.so`。Qt 示例在 `../examples/qt_skeleton_viewer/main.cpp`。所有 API 均使用 C ABI；`HexPartnerHandle` 借用 glove、mocap 和可选的 external 快照会话，必须先销毁 partner，再销毁这些句柄。

## 1. 连接、断开和重连

```cpp
HexGloveHandle glove = hex_glove_create();
HexMocapHandle mocap = hex_mocap_create(glove);
HexExternalHandle frame = hex_external_create(glove, mocap);
HexPartnerConfig cfg{};
cfg.struct_size = sizeof(cfg);
hex_partner_config_init(&cfg);
cfg.auto_detect = 1;                 // 搜索系统串口；也可填 preferred_port_utf8
cfg.auto_reconnect = 1;
snprintf(cfg.profile_dir_utf8, sizeof(cfg.profile_dir_utf8), "%s", "profiles");
HexPartnerHandle partner = hex_partner_create(glove, mocap, frame, &cfg);
hex_partner_connect(partner, monotonic_ms);
// Qt 工作线程的定时器：每轮只调用一次 tick，再读取状态/事件/快照。
hex_partner_tick(partner, monotonic_ms);
hex_partner_disconnect(partner);
hex_partner_destroy(partner);
hex_external_destroy(frame);
hex_mocap_destroy(mocap);
hex_glove_destroy(glove);
```

`hex_partner_tick` 按需搜索串口、探测接收器、重试连接、推进 mocap 和外部快照。不要同时自行调用 `hex_glove_start/abort`、`hex_mocap_update` 或 `hex_external_update`。单个接收器可同时包含左、右手；`online_hand_mask` 与 `streaming_hand_mask` 的 bit 0/1 分别代表左/右。搜索的是**接收器串口**，不是独立枚举每只无线手套。

`HexPartnerStatus.phase`：0 停止、1 搜索、2 连接/探测、3 接收器已开但等待数据、4 数据输出中、5 曾有数据而本轮中断超时、6 正在重试、7 失败。`last_fresh_ms[2]`、`fresh_hand_mask`、`attempts`、`failures` 可用于界面诊断。默认断流门限为 1000 ms，可在 `cfg.data_timeout_ms` 调整。断流不会把“接收器端口仍开着”错误报告为“接收器物理断开”。

每次状态变化产生递增的 `event_sequence`；用 `hex_partner_get_event(partner, last_sequence, &event)` 取后续事件，返回 1 有事件、2 没有新事件、0 参数错误。SDK 保存最近 32 条事件；接入方应每轮取完。事件在调用 `tick` 的线程产生，**没有后台回调**。Qt 建议在专用工作线程调用全部 partner/glove/mocap/external API，再用 Qt 信号向界面线程传值。

`hex_partner_error_string(status.last_error)` 返回可显示的英文原因。端口打开失败可查询底层阶段及系统错误码；Windows 拒绝访问仍可能是权限或占用，不能只凭该码区分。异常关闭、接收器已打开后一直无数据、此前输出过的手停止发帧会产生对应断连/数据中断事件。自动重连针对接收器端口；手套断电但接收器仍打开时，保持端口并等待手套重新发帧。

`status.operation_sequence/kind/state/error` 是最近一次高层操作的 ID、种类、状态和最终原因。异步 `connect` 与 `calibration_begin` 返回 1 表示受理，之后靠 `tick` 推进到成功或失败；不能把返回 1 当作已连接或标定完成。同步档案操作与断开也写入这组字段。事件包含关联的 `operation_sequence`；若多个操作相继执行，应用应保存自己的操作 ID 和结果。`hex_partner_get_timing` 提供 `observed_hz[2]`、`last_fresh_age_ms[2]`、`hand_arrival_gap_ms`，单位分别为 Hz 和毫秒；`-1` 表示还没有有效样本。这些量来自宿主接收时间，不是传感器采样率、真实端到端延时或硬件同步证明。
同一个 partner 同时只保留一项最近操作结果；连接或标定处于 RUNNING 时，不要发起另一项档案操作。若调用返回 0，先查询 `last_error`；当前已保存 Windows/Linux 原始系统错误号，但接收器专用驱动兼容性仍需硬件厂商资料确认。

## 2. 设备标识与校准文件

`hex_partner_get_status` 返回当前串口 `port_utf8`、左右在线位和每手可读的 `device_code[2][3]`。`device_code` 只有两个字符，**不是已验证的物理唯一序列号**。当前配置按“接收器串口＋左右手槽位”绑定：一个 `.hexprofile` 文件包含同一接收器的左右手数据，文件名是端口 UTF-8 字节的十六进制表示，例如 `COM3` → `receiver-434f4d33.hexprofile`。用 `hex_partner_profile_path` 可查询实际路径。

新版本在打开串口后自动发送读 ID 指令。应用可用 `hex_glove_get_device_id(glove, id, sizeof(id))` 从 partner 借用的 glove 句柄读取六位**接收器** ID；未收到回复时返回 `0`，必要时用 `hex_glove_request_device_id(glove)` 重发。该 ID 不属于左右手套各自的独立序列号；现有 `.hexprofile` 自动绑定方式仍按端口与左右槽位，尚未迁移至 ID 命名。

设置 `cfg.profile_dir_utf8` 后，`cfg.auto_load_profile=1` 会在接收器打开时自动读取该端口的现有 profile；没有文件时继续等待数据，不伪造校准结果。`cfg.auto_save_profile=1` 会在本接口启动的 R 或完整标定成功后保存 profile；也可手动调用 `hex_partner_profile_save/load`。保存时先写新文件并保留旧文件备份，再替换目标。若接收器串口号改变，或在相同端口更换手套，请由上位机重新选择/确认配置；当前硬件字段不足以自动证明是同一物理设备。保存失败会单独发出事件，不能把“标定成功”误认为“已持久化”。

## 3. R 与完整标定

`hex_partner_calibration_begin(partner, mask, mode, seconds)`：`mask=1` 左、`2` 右、`3` 双手；双手需要两侧都在 `streaming_hand_mask` 中，然后**同时**启动 R。`mode=HEX_PARTNER_CALIBRATION_R` 是普通 R；`HEX_PARTNER_CALIBRATION_FULL` 调用完整 SDK 里的多步流程，包括 R、活动范围、拇指、对指与环长。完整流程有需要用户确认的步骤，UI 应显示 `hex_partner_get_calibration` 的 `step[2]`、`progress[2]`、`prompt_utf8[2]`，用户完成动作后调用 `hex_partner_calibration_advance`。环长步骤需要实际测量值，使用 `hex_partner_calibration_commit_ring`；不能用演示常数提交真实校准。

`hex_partner_calibration_cancel` 取消当前 R/多步事务；`retry` 以相同的手、模式和 R 秒数重新开始。每轮 `tick` 更新 `HexPartnerCalibrationStatus.result`：0 空闲、1 进行中、2 成功、3 取消、4 失败。R 失败及完整流程失败分别有错误码和事件；SDK 原标定流程没有结构化的每一步细分失败原因，具体操作提示仍以 `prompt_utf8` 为准。`hex_partner_profile_save` 会保存当前结果；重新 R 后还应在新鲜伸直中立姿态调用 `hex_external_calibrate_angles` 建立角度零位。

## 4. 双手骨骼与既有 UDP 字段

一次 `tick` 后分别以 `hand=0/1` 读取 `hex_mocap_get_display_pose`，每手各 21 节点；检查 `tracked_mask` 再显示。`local_position` 相对父节点，单位米；`world_position` 为世界位置，单位米。原上位机的 UDP 位置是相对手掌的厘米，不能将 `local_position` 直接拷贝到旧字段。`hex_partner_encode_legacy_udp(partner,channel,utc_ms,buf,capacity)` 复用 SDK 自身的旧编码器：0 为 `pitch/side/roll/two_pitch/end_pitch` 角度，1 为位置加旋转，2 为位置，3 为旋转。可先以空缓冲和 0 容量查询含终止零字节的大小，再分配缓冲编码。`utc_ms` 须由应用提供真实 Unix UTC 毫秒；输出 JSON 的时间字段不是 SDK 单调时钟。仅在 `fresh_hand_mask` 非 0 的当前轮调用，避免重发陈旧帧。该函数负责序列化，不负责打开 UDP 套接字或网络发送。

每手本地骨骼都遵守 Unity 坐标约定和相同节点顺序；左右局部轴、镜像修正及拇指参数不同。普通精确角的单位为度，完成伸直零位校准后应接近 0°，但允许负值。`finger[5][4]` 无独立拇指 CMC；需通过 Angle Hub 读取。旧 UDP 角字段维持原格式，不能直接等同于 Hub `flexion[21]`。

## 5. 运行环境和已知边界

- 当前交付的 Windows 库为 **x64、MinGW** 构建，附带 `libHexaCercleGlove.dll.a` 与 `libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`。Qt 6.5 MinGW 套件可链接该导入库；Qt MSVC 套件需要另行验证并提供 MSVC 导入库，或通过 `QLibrary` 动态加载 C ABI。SDK CMake 源码有 MSVC 分支，但这不等于已交付 MSVC 构建。
- SDK 通过 Windows 系统串口 API 打开接收器；需要设备在系统中出现 COM 端口。端口以独占方式打开：若旧上位机占用，先关闭旧程序，再连接。本 SDK 不要求运行 `HexacercleGlove.exe`。接收器型号、专用 USB 驱动及商业授权条款应以硬件供方资料核定；代码内未发现可据此承诺的完整清单。
- `hex_partner_tick` 所有调用由接入方线程驱动；数据频率取决于设备发帧与调用频率，不应把示例循环 20 Hz 当作硬件帧率或端到端延迟保证。真正性能需在目标接收器和 Windows 机器上测量。
