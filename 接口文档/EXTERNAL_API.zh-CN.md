# HexaCercle 手套 SDK｜API 概览

> **接口类型**：本地 C ABI（Windows DLL／Linux SO）  
> **适用对象**：C++17、Qt 6.5+、Python `ctypes` 接入方  
> **文档版本**：2026-10-15；函数声明以随包 `library/include/hex_glove/` 为准

[文档入口](README.zh-CN.md) · [接入指南](EXTERNAL_INTEGRATION.zh-CN.md) · [函数参数](EXTERNAL_API_PARAMETERS.zh-CN.md) · [工作流接口](PARTNER_INTEGRATION.zh-CN.md) · [交付状态](WINDOWS_CPP_SDK_DELIVERY_STATUS.zh-CN.md)

## 本页目录

1. [API 能力一览](#api-能力一览)
2. [接收器设备 ID](#接收器设备-id)
3. [自动连接与工作流](#自动连接与工作流)
4. [统一单帧快照与接入诊断](#统一单帧快照与接入诊断)
5. [连接与断开](#1-连接与断开)
6. [设备基础信息](#2-设备基础信息)
7. [21 节点与校准](#3-骨骼会话与-21-节点)
8. [角度与时间戳](#5-角度与数据导出)

## API 能力一览

| 能力 | 主要接口 | 读取结果／使用条件 |
| --- | --- | --- |
| 自动搜索、连接、重连 | `hex_partner_connect`、`hex_partner_tick` | 通过 `HexPartnerStatus.phase` 判断最终阶段；连接请求受理不等于已收到数据。 |
| 手动打开与关闭串口 | `hex_glove_start`、`hex_glove_abort` | 已知端口且应用自行管理连接时使用。 |
| 六位接收器设备 ID | `hex_glove_get_device_id` | 打开端口后自动请求；有效回复到达才返回 `1`。 |
| 电量、信道、固件与硬件 | `hex_glove_get_device_snapshot`、`hex_glove_get_hardware_info` | 检查在线、回复和 `valid` 字段。 |
| R 与校准档案 | `hex_mocap_start_r_capture`、`hex_mocap_profile_load` | 双手 R 同轮采集；档案当前按端口和左右槽位管理。 |
| 左右手各 21 骨骼节点 | `hex_mocap_get_display_pose` | 只有 `tracked_mask` 对应的节点代表当前实时跟踪。 |
| 15 项关节角与统一快照 | `hex_external_update`、`hex_external_get_frame` | 检查 `fresh_hand_mask` 与 `angle_valid_mask`；角度单位为度。 |
| 连接与接收诊断 | `hex_mocap_get_status`、`hex_glove_get_receive_diagnostics` | 区分无连接、等待数据、未完成 R、持续丢帧。 |

**调用起点**：首次接入先读[接入指南](EXTERNAL_INTEGRATION.zh-CN.md)；查某个函数的参数、输出与错误时读[函数参数手册](EXTERNAL_API_PARAMETERS.zh-CN.md)。本地 SDK 不使用 HTTP 域名、请求方法或云端签名。

## 接收器设备 ID

SDK 成功打开串口并清空旧接收缓存后，自动发送 `A0 FF FF 28 01 05 FF 0A`。对回复 `A0 01 0F 28 05 31 31 34 32 30 35 01 02 01 00 82 0A`，SDK 校验完整帧与异或校验字节 `82`，将六个 ASCII 字节 `31 31 34 32 30 35` 解码为接收器设备 ID `114205`。该 ID 属于**接收器**，不是左右手各自的独立 ID，也不等同于原有两字符 `device_code`。

包含 `hex_hardware_info_api.h`，用 `char device_id[7]{}; hex_glove_get_device_id(glove, device_id, sizeof(device_id))` 读取。返回 `1` 表示收到有效 ID，`0` 表示尚未收到、串口未打开或缓冲区不足；不要把 `0` 当作设备 ID 为零。若一直未收到，可调用 `hex_glove_request_device_id(glove)` 重新发送，再轮询读取；该函数返回 `1` 只表示指令写入串口。断开/重连会清空旧 ID。高层 partner 创建时仍可通过其借用的 `glove` 句柄读取。当前校准档案的自动绑定仍按接收器端口和左右手槽位，设备 ID 不会自动迁移旧档案。

## 自动连接与工作流

**串口错误与发流控制：** `hex_glove_get_last_open_error(glove,&stage,&platform_code)` 可读取最近一次底层打开失败阶段和原始 Windows／Linux 错误码。高层 `HexPartnerStatus.port_error_stage/platform_error_code` 保留最近一次探测结果，`last_error` 区分不存在、拒绝访问或占用、串口配置失败、启动命令写入失败。Windows 错误 5 不能单独判定是否被占用。`hex_partner_stream_control(partner,1/0)` 对已打开接收器发送开始/停止流命令；返回 1 仅确认串口写入，开始后仍须等新帧。停止后阶段为 STREAM_PAUSED，重连默认恢复发流；标定进行中不能暂停流。自动重试时先发 DISCONNECTED，再发 RETRYING 事件。

包含 `hex_partner_api.h`。此层统一管理串口搜索、双手新帧、断流/重连、标定和档案。调用方创建 glove、mocap、可选 external 后创建 partner；`connect` 后定时 `tick`，最后 `disconnect`、销毁 partner 和借用句柄。不能在 partner 运行期间自行调用底层 `hex_glove_start/abort` 或 `hex_mocap_update`。所有调用由同一应用线程串行执行。

| 函数 | 作用 |
| --- | --- |
| `hex_partner_get_api_version` | 取得工作流 ABI 版本，当前 `0x00030000`。 |
| `hex_partner_config_init` | 初始化带 `struct_size` 的探测、重连、断流及档案配置。 |
| `hex_partner_create` / `hex_partner_destroy` | 借用/释放高层控制器；不转移底层句柄所有权。 |
| `hex_partner_connect` / `hex_partner_disconnect` | 启动搜索连接事务 / 停止重连并关闭串口。连接返回 1 只表示受理。 |
| `hex_partner_stream_control` | 发送开始或停止发流命令，成功表示串口写入，非设备确认。 |
| `hex_glove_get_last_open_error` | 读取底层串口打开失败阶段和操作系统原始错误码。 |
| `hex_partner_tick` | 根据非递减单调毫秒推进探测、更新、超时、重连和标定。 |
| `hex_partner_get_status` | 读取阶段、端口、双手在线/新帧/流位、短设备码、最后错误和最近操作最终状态。 |
| `hex_partner_get_timing` | 读取主机观测双手 Hz、距上次新帧的毫秒及双手到达差；没有样本为 -1。 |
| `hex_partner_get_event` | 按序号读取最近 32 条连接、断流、重试、标定等事件；1 有新事件、2 无、0 参数错。 |
| `hex_partner_error_string` | 将数值错误码转换为英文原因。 |
| `hex_partner_calibration_begin` / `hex_partner_calibration_cancel` / `hex_partner_calibration_retry` | 开始、取消和重试 R 或完整引导标定；双手 mask 3 需同时有数据。 |
| `hex_partner_calibration_advance` / `hex_partner_calibration_commit_ring` / `hex_partner_get_calibration` | 推进引导步骤、提交真实环长测量、读取步骤/提示/进度/最终结果。 |
| `hex_partner_profile_save` / `hex_partner_profile_load` / `hex_partner_profile_path` | 保存、加载和查询当前接收器端口绑定的双手档案。 |
| `hex_partner_encode_legacy_udp` | 从最新帧编码既有 UDP JSON。channel 0 角度、1 姿态、2 位置、3 旋转；UTC Unix 毫秒由调用方提供。 |

`status.operation_sequence/kind/state/error` 表示最近操作的 ID、类型、RUNNING/SUCCEEDED/FAILED/CANCELLED 和失败原因；事件中有对应操作 ID。`phase` 区分搜索、连接、等待数据、流、断流、重试及失败。初次开口后无数据超过 `data_timeout_ms` 也进入断流。事件由调用 `tick` 的线程产生，没有后台用户回调。`device_code` 只有两个字符，不代表唯一物理序列号；档案按“接收器端口＋左右槽位”管理。
同一控制器一次只保留一项最近操作结果；连接/标定尚在 RUNNING 时不要并发发起档案保存或加载。调用返回 0 时查询 `last_error`，异步最终结果则查询 `operation_state/error`。

位置映射请注意：`HexMocapDisplayPose.local_position` 是相对父节点的米，旧 UDP 位置是相对手掌的厘米。旧 UDP 角 `pitch/side/roll/two_pitch/end_pitch` 与 Hub 关节屈伸并非同一字段。精确角度单位为度，伸直校准后接近 0°，允许部分负角；精确 `finger[5][4]` 不含单独 CMC，CMC 通过 Angle Hub 获取。两手共享 Unity 坐标和关节顺序，但局部轴及拇指修正各自处理。详见 [接入与诊断流程](PARTNER_INTEGRATION.zh-CN.md)。

本文列出对外接入能力：连接/断开串口、设备状态与版本、信道、电量、R 校准、加载校准文件、每只手的 21 节点骨骼数据、精确角度与 JointAngleHub 角度，以及接入诊断、ABI 检查和统一单帧快照。CSV、JSON 和实时流的导出方法见[对外接入指南](EXTERNAL_INTEGRATION.zh-CN.md)。库中另有内部导出函数，不属于本次对外接口承诺。

面向上位机的一体化搜索、重连、状态事件、标定流程与 profile 自动管理，参见[上位机对接指南](PARTNER_INTEGRATION.zh-CN.md)和 `hex_partner_api.h`。它复用同一套骨骼与快照 API；接入方应选择一种更新循环，不要在 `hex_partner_tick` 后额外调用 `hex_external_update`。

原生库：Windows `HexaCercleGlove.dll`，Linux `libHexaCercleGlove.so`。对外使用 C ABI，包含 `library/include/hex_glove/hex_glove_api.h`、`hex_mocap_api.h`、`hex_angles_api.h`、`hex_external_api.h`；硬件信息结构及查询声明位于 `hex_hardware_info_api.h`。所有句柄由创建它的应用负责释放。

## 统一单帧快照与接入诊断

`hex_external_api.h` 提供外部会话。它引用现有 glove/mocap 句柄，不取得所有权；先销毁外部会话，再销毁 mocap 和 glove。所有操作在同一线程调用。下表中返回 `int` 的函数以 `1` 表示成功、`0` 表示失败。

| 函数 | 作用与用法 |
| --- | --- |
| `hex_external_create(glove, mocap)` | 创建外部会话；mocap 必须由同一个 glove 创建。 |
| `hex_external_destroy(external)` | 释放外部会话，不关闭串口，也不释放原始句柄。 |
| `hex_external_get_api_version()` | 返回此套新接口的版本；当前 `0x00010000`。 |
| `hex_external_profile_load(external, path)` | 加载 `.hexprofile`，并清除上次捕获及角度零位；失败时可查 `PROFILE_LOAD_FAILED`。 |
| `hex_external_start(external, port)` | 打开串口；包装层失败返回 `PORT_OPEN_FAILED`。进一步用 `hex_glove_get_last_open_error` 查询打开阶段与系统原始码。重新连接会清除角度零位。 |
| `hex_external_update(external)` | 推荐的循环入口：推进一次 mocap，并缓存该轮左右手 21 节点与 Hub 角度。 |
| `hex_external_capture(external)` | 仅用于应用已自行调用过一次 `hex_mocap_update` 的情况；从当前求解状态缓存快照，不再次推进。 |
| `hex_external_get_frame(external, &frame)` | 复制最新缓存，不推进求解；调用方先设置 `frame.struct_size=sizeof(frame)`。 |
| `hex_external_calibrate_angles(external, hand_mask)` | 传感器 R 完成后，保持手指伸直中立，给所选手同时建立 Hub 与精确角度零位；掩码 1 左、2 右、3 双手。 |
| `hex_external_get_last_error(external, &error)` | 读取最近一次新接口调用的错误码和 UTF-8 信息；设置 `error.struct_size=sizeof(error)`。 |
| `hex_external_error_string(code)` | 将错误码转换为静态英文字符串，不需释放。 |

`HexExternalFrame` 中 `update_index` 是本会话捕获次数；`host_timestamp_ms` 是宿主单调时钟毫秒数，**不是 Unix UTC**。`pose[0/1]` 分别是左/右手各 21 节点，保留各自的设备 `timestamp_ms` 和 `frame_sequence`。`tracked_hand_mask` 表示该手有跟踪节点；`fresh_hand_mask` 表示本轮确实收到该手的新帧；`angle_valid_mask` 表示该手 15 项 Hub 屈伸角有效。三个掩码均以 bit 0 表示左手、bit 1 表示右手。输出时必须检查相应有效位，导出实时新数据还应检查 `fresh_hand_mask`。左右手在一次宿主更新后缓存，**不声称两个设备在硬件层同步采样**。

`flexion_deg[hand][15]` 的固定顺序为 `thumb_cmc, thumb_mcp, thumb_ip, index_mcp, index_pip, index_dip, middle_mcp, middle_pip, middle_dip, ring_mcp, ring_pip, ring_dip, little_mcp, little_pip, little_dip`；单位为度。角度正负应按具体关节和左右手校准结果解释，不能一概假定正值。R 校准与几何角度零位是两步；重新 R 后请重新调用角度校准。CSV/JSON 示例还写入导出时的 `utc_timestamp_ms`，用于和系统时间对齐。

错误码 `HexExternalErrorCode`：`0 OK`、`1 INVALID_ARGUMENT`、`2 BAD_STRUCT_SIZE`、`3 PORT_OPEN_FAILED`、`4 PROFILE_LOAD_FAILED`、`5 UPDATE_FAILED`、`6 NO_CAPTURED_FRAME`、`7 NO_TRACKED_HAND`、`8 ANGLE_CALIBRATION_FAILED`、`9 NO_FRESH_FRAME`。新错误接口只记录 `hex_external_*` 包装调用的错误；旧版 API 的 `0` 返回值不会自动写入这里，不能据此推断旧调用的失败原因。

### 连接状态、接收诊断与 ABI 检查

| 函数 | 作用与用法 |
| --- | --- |
| `hex_mocap_get_status(mocap)` | 返回状态位；`NO_GLOVE` 表示未连接，`NO_SYNC_FRAME` 表示等待同步数据，`R_NOT_DONE` 表示 R 未完成。按位测试，不能只判断 `update` 返回值。 |
| `hex_mocap_get_fresh_hand_mask(mocap)` | 最近一次 `update` 真正收到新帧的手；1 左、2 右。 |
| `hex_glove_get_receive_diagnostics(glove, &diag)` | 接收字节、帧数、校验不匹配和成帧错误累计值；调用前设置 `diag.struct_size=sizeof(diag)`。比较前后差值可判断持续丢帧。 |
| `hex_mocap_get_abi_info(&info)` | 填入已加载库的版本、指针大小、输入结构大小/字段偏移和骨骼关节数；调用前设置 `info.struct_size=sizeof(info)`。 |
| `hex_mocap_get_capabilities()` | 返回功能位集合；按 `mocap_abi.h` 中的 `HEX_MOCAP_CAP_*` 位检查所需能力。 |

`HexMocapAbiInfo` 的 `display_joint_count` 应为 21；Python/Qt 还应核对 `pointer_size`、相关结构大小和版本。`get_capabilities` 表示底层可调用能力，不表示所有内部功能都在本外部包中承诺开放。诊断计数器是累计值，帧数不增长但字节增加时应结合 `framing_errors` 与 `checksum_mismatches` 判断。连接状态与数据新鲜度也要结合设备快照、R 状态使用。

## 1. 连接与断开

| 函数 | 作用与用法 |
| --- | --- |
| `HexGloveHandle hex_glove_create(void)` | 创建手套会话；返回空指针表示失败。此时尚未打开串口。 |
| `int hex_glove_start(HexGloveHandle, const char* port_name)` | 用 UTF-8 端口名打开串口并启动接收，例如 `COM3`、`/dev/ttyUSB0`；返回 1 成功、0 失败。 |
| `void hex_glove_abort(HexGloveHandle)` | 正常断开：停流、停止接收并关闭串口。 |
| `void hex_glove_destroy(HexGloveHandle)` | 释放会话；应在销毁 mocap 会话后调用。 |

`hex_glove_shutdown` 向手套发送**关机**命令；日常“断开连接”请使用 `hex_glove_abort`。串口传输波特率由 SDK 按设备协议设置。

## 2. 设备基础信息

| 函数 | 作用与用法 |
| --- | --- |
| `int hex_glove_get_device_snapshot(HexGloveHandle, HexGloveDeviceSnapshot* out)` | 一次读取左右在线、扩展节点在线、布局、产品提示、信道、电量及原始硬件字节。返回 1 表示结构读取成功，仍须检查各有效字段。 |
| `int hex_glove_get_hardware_info(HexGloveHandle, int hand, HexGloveHardwareInfo* out)` | 读取某只手解析后的固件、硬件版本、设备编码、传感器状态；调用前设置 `out->struct_size=sizeof(*out)`，并检查 `out->valid`。 |
| `void hex_glove_send_get_route(HexGloveHandle)` | 请求设备返回当前信道；连接后调用一次即可，不要每帧发送。 |
| `int hex_glove_is_route_received(HexGloveHandle)` | 判断信道回复是否已收到。 |
| `int hex_glove_get_route(HexGloveHandle)` | 读取信道整数；先确认已收到回复。 |
| `uint64_t hex_glove_get_rx_bytes(HexGloveHandle)` | 读取本次连接接收的字节数，可用于诊断是否有持续数据。 |
| `uint32_t hex_mocap_get_api_version(void)` | 读取 SDK API 版本号；这是**软件接口版本**，不同于手套固件/硬件版本。 |

`hand=0` 为左手，`hand=1` 为右手。`HexGloveDeviceSnapshot.left_enabled/right_enabled` 是当前在线判断；`batt_left/batt_right` 为电量估算，离线时不要将数值当作实时电量。`route_received` 为 0 时，不要把 `route` 当作已确认信道。`layout_left/right` 常见值为 7、11，0 表示未知。

`HexGloveHardwareInfo.software_major/software_minor` 是固件版本；`hardware_major/hardware_minor` 是硬件版本。只有 `hex_glove_get_hardware_info` 成功且 `valid=1` 时显示它们。原始 18 字节不建议由接入方自行猜测字节顺序。

## 3. 骨骼会话与 21 节点

| 函数 | 作用与用法 |
| --- | --- |
| `HexMocapHandle hex_mocap_create(HexGloveHandle glove)` | 创建与实时手套绑定的骨骼求解会话；返回空指针表示失败。 |
| `int hex_mocap_update(HexMocapHandle)` | 每帧调用一次，推进接收数据、R 采集和骨骼求解。返回成功仍须检查输出有效位。 |
| `int hex_mocap_get_display_pose(HexMocapHandle, int hand, HexMocapDisplayPose* pose)` | 读取某只手的 21 节点姿态；调用前设置 `pose->struct_size=sizeof(*pose)`。用 `valid_mask` 检查模型节点，用 `tracked_mask` 检查当前真实跟踪节点。 |
| `void hex_mocap_destroy(HexMocapHandle)` | 销毁骨骼会话；不会销毁手套会话。 |

`HexMocapDisplayPose` 为每只手提供：

- `world_position[21]`：世界位置，单位米；
- `world_rotation[21]`、`local_rotation[21]`：世界/局部四元数，顺序 `(x,y,z,w)`；
- `local_position[21]`、`local_scale[21]`：相对父节点的位置和比例；
- `world_matrix[21][16]`：列主序世界矩阵；
- `timestamp_ms`、`frame_sequence`、`status_flags`：本次结果的时间、序号、状态。

节点顺序固定：`0` 手掌；`1–4` 拇指；`5–8` 食指；`9–12` 中指；`13–16` 无名指；`17–20` 小指。每指四个显示节点。**21 节点是每只手各一组**，左、右手分别以 `hand=0/1` 查询。`get_display_pose` 成功只说明输出结构已填充；`tracked_mask=0` 时不能把静止模型预览当成实时手部动作。

## 4. 加载校准数据与 R 校准

| 函数 | 作用与用法 |
| --- | --- |
| `int hex_mocap_profile_load(HexMocapHandle, const char* path_utf8)` | 从 `.hexprofile` 文件加载骨架定义和校准配置。路径为 UTF-8；返回 1 成功、0 失败。建议在开始逐帧求解前调用。 |
| `int hex_mocap_start_r_capture(HexMocapHandle, int hand_mask, float seconds)` | 对当前有效手开始 R 零位采样；`hand_mask=1` 左、`2` 右、`3` 双手，`seconds` 为采样时长。开始后继续调用 `hex_mocap_update` 推进新帧。 |
| `int hex_mocap_get_r_capture_info(HexMocapHandle, HexMocapRCaptureInfo* out)` | 查询 R 采样状态和进度；调用前设置 `out->struct_size=sizeof(*out)`。 |

R 状态：`0` 空闲、`1` 采样、`2` 完成、`3` 失败、`4` 取消。`progress` 为进度，`fresh_hand_mask` 标识收到新帧的手。校准文件可能已经保存 R 基准。普通 R 在已学习轨迹存在时用于手掌朝向对齐；它不保证重建传感器安装零位。若重新佩戴改变了安装位置，需要取得与当前佩戴状态匹配的校准档案；内部另有重戴 R 入口，但不在本版外部基础接口清单。R 零位校准不等于重新执行完整手势/对指校准。

加载 `.hexprofile` 后，请直接使用其中的模型和校准结果，不要再用演示用的 `hex_mocap_skeleton_definition_init` 覆盖。若没有校准文件，可以用该初始化函数生成合成骨架做数据链路演示；它不代表真实模型绑定和完整校准效果。

## 5. 角度与数据导出

### 最终骨骼角度

角度输入应使用接触修正和手型适配后的最终 `HexMocapDisplayPose`。精确角度由 mocap 会话自动更新；完成传感器 R 且取得有效跟踪姿态后，调用 `hex_mocap_calibrate_angles(mocap, hand_mask)` 建立**另一套几何角零位**。之后逐帧 `hex_mocap_update`，用 `hex_mocap_get_precise_angles(mocap, hand, &snapshot)` 读取。`snapshot.struct_size` 须设为 `sizeof(snapshot)`，并检查 `calibrated`、`valid_mask` 与对应 `pose.tracked_mask`。R 本身不等于精确角零位校准；重新 R 后应再次建立角度零位。

| 函数 | 作用与用法 |
| --- | --- |
| `hex_mocap_calibrate_angles(mocap, hand_mask)` | 使用当前已跟踪姿态校准精确角度。掩码 `1` 左、`2` 右、`3` 双手；双手模式要求两手都有有效姿态。 |
| `hex_mocap_get_precise_angles(mocap, hand, &snapshot)` | 读取左手 `0` 或右手 `1` 的最新精确角度快照。 |
| `hex_angle_hub_config_init(&config)`、`hex_angle_hub_state_init(&state)` | 初始化 21 槽关节定义与调用方持有的状态；先将两个结构的 `struct_size` 设为各自 `sizeof`。 |
| `hex_angle_hub_calibrate(&config, &state, &pose, nullptr)` | 以当前最终骨骼姿态建立 Hub 零位；左右手分别维护一份 `state`。 |
| `hex_angle_hub_update(&config, &state, &pose, dt_seconds, nullptr, nullptr)` | 每个有效帧更新 Hub 角度；`dt_seconds` 是本次与上次更新的秒数。读取 `state.valid_mask` 中对应位的 `flexion[21]`、`abduction[21]`。 |

`HexAnglesSnapshot.finger[5][4]` 的行依次为拇指、食指、中指、无名指、小指，列依次为 MCP、PIP、DIP、spread；**没有单独的拇指 CMC 项**。常用对外字段 `thumb_mcp` 可取 `finger[0][0]`；`thumb_ip` 应取 Hub 的第 3 槽，不能把精确数组的 PIP/DIP 直接改名成 IP。Hub 槽位：`1` thumb_cmc、`2` thumb_mcp、`3` thumb_ip；`5/6/7` index_mcp/pip/dip，`9/10/11` middle，`13/14/15` ring，`17/18/19` little。若要同一算法口径的每指角度，全部从 Hub `state.flexion` 读取；不要把 Precise 和 Hub 值拼成一组而不标明算法来源。

关节屈伸、外展和精确角度均以**度（°）**表示。完成伸直中立位的几何角校准后，读数应接近 `0°`；校准时已弯曲的姿态会成为零位。精确屈伸角按屈曲为正的意图处理，但带符号夹角、左右拇指符号修正和部分负角意味着**不能保证所有输出均非负**。两手使用相同 Unity 坐标约定和关节顺序，但局部轴、镜像修正和拇指参数有差异，不应直接对一侧数值做简单取反来推断另一侧。Hub 的拇指 CMC/MCP 沿用世界方向相对零位的算法，与 Precise 的掌局部计算不同；转腕时不要将两套读数视为等价。

### 带时间戳导出

每次成功 `hex_mocap_update` 后，按手读取 `pose.timestamp_ms`、`pose.frame_sequence` 和有效的 Hub 角度，再由接入程序写 CSV、JSON Lines 或通过实时传输发送。`timestamp_ms` 是宿主**单调时间毫秒**，适合帧排序与间隔计算；若需要 UTC 时间，应由宿主同时记录 UTC Unix 毫秒，并分别命名 `utc_timestamp_ms`。建议字段顺序：`timestamp_ms,utc_timestamp_ms,frame_sequence,hand,thumb_cmc,thumb_mcp,thumb_ip,index_mcp,index_pip,index_dip,middle_mcp,middle_pip,middle_dip,ring_mcp,ring_pip,ring_dip,little_mcp,little_pip,little_dip`；所有角度单位为度。仅输出 `pose.tracked_mask` 与 `state.valid_mask` 都有效的关节，断流时不要重复上一帧当作新数据。CSV/JSON 是宿主对 SDK 实时结果的序列化；接入示例见指南。

## 6. 通用约定

- 同一个 mocap 会话的配置、`update`、读取和销毁应在同一调用线程串行执行；SDK 内部另有串口接收线程。
- 退出顺序：`hex_mocap_destroy` → `hex_glove_abort` → `hex_glove_destroy`。
- 带 `struct_size` 的输出结构先清零并设置为 `sizeof(结构)`；保持头文件默认内存对齐，不使用 `pack(1)`。
- 实时 `timestamp_ms` 是宿主单调时间，不是 Unix UTC 时间。
- 仅消费 `tracked_mask` 对应的实时骨骼；模型有效位与跟踪有效位含义不同。

> 版本提示：Windows 库已同步至当前源码构建；随包 Linux 库已于 2026-10-15 从当前源码重新构建。外部接口以本文件和精简头文件为准，但 Linux 运行行为须在目标环境验证。




