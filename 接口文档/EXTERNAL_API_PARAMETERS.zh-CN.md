# 对外 SDK 函数参数手册

**10.27 参数核对**：对外函数名与 C 签名沿用 10.16，以下参数表继续适用；运行效果及档案版本差异见[10.16 → 10.27 接口变更说明](../SDK_CHANGELOG_10.16_TO_10.27.zh-CN.md)。新内部接口不在本手册的开放范围。

本手册与 [对外 API 说明](EXTERNAL_API.zh-CN.md) 配套，按交付头文件列出接入所需函数的**完整 C 签名、每个参数、返回值与调用条件**。具体结构体字段和 21 节点顺序以随包 `library/include/hex_glove/` 中的头文件为准。库中还有内部函数，本手册只覆盖对外接入接口。

> **阅读顺序**：[文档入口](README.zh-CN.md) → [接入指南](EXTERNAL_INTEGRATION.zh-CN.md) → 本页参数与返回值。  
> **适用版本**：2026-10-27 配套 Windows x64／Linux x64 库；调用前应按 `hex_mocap_get_abi_info` 核对结构布局。

## 本页目录

- [共同约定](#1-共同约定)：参数类型、左右手、掩码、编码、时间与线程。
- [关键接口详解](#关键接口详解)：按“描述 → 输入 → 输出 → 示例 → 失败处理”快速查常用 API。
- [完整函数索引](#2-高层连接与标定hex_partner_apih)：按头文件覆盖所有对外函数。
- [调用顺序与错误处理](#7-常见调用顺序与错误处理)：生命周期与错误来源。

## 1. 共同约定

| 名称 | 约定 |
| --- | --- |
| `handle` / `glove` / `mocap` / `external` | 由对应 `create` 返回的非空句柄。`partner` 借用已有句柄，不负责销毁它们。 |
| `hand` | `0` 左手，`1` 右手。 |
| `hand_mask` | 位 0 左手=`1`，位 1 右手=`2`，双手=`3`。双手 R 在同一次采集中完成。 |
| `port_utf8` / `path_utf8` | 以 `\0` 结尾的 UTF-8 字符串。Windows 端口如 `COM3`，Linux 如 `/dev/ttyUSB0`；档案路径为 `.hexprofile`。 |
| `struct_size` | 调用带此字段的输出结构前先清零，再设为 `sizeof(该结构)`；不要使用 `#pragma pack(1)`。初始化函数也要求有效结构体指针。 |
| 返回 `int` | 除特别说明，`1` 成功，`0` 失败。成功读取结构体也不等于当前有有效手或新帧，须检查有效位。 |
| 时间 | `monotonic_ms`、`host_timestamp_ms` 和骨骼 `timestamp_ms` 是主机单调时钟毫秒；`utc_ms` 是调用方提供的 Unix UTC 毫秒。 |
| 线程 | 同一套句柄的配置、更新、读取、销毁由一个应用线程串行调用。 |

有两种更新路径：高层 `hex_partner_tick` 管理连接、重连与标定；直接接入则调用 `hex_mocap_update` 或 `hex_external_update`。同一轮不要混用这些推进函数。使用 partner 时，底层 glove/mocap/external 都由它借用，应用不要自行 `start`、`abort` 或 `update`。

## 关键接口详解

以下五组接口按接入顺序排列。每组均采用固定结构；后文[完整函数索引](#2-高层连接与标定hex_partner_apih)补齐其他函数。示例使用 C++17，返回值都是直接调用原生 C ABI 的结果。

### 打开串口：`hex_glove_start`

**接口描述**：打开指定接收器串口、启动数据接收，并自动发送一次读设备 ID 指令。高层 partner 用户由 `hex_partner_connect/tick` 管理此步骤。

```c
int hex_glove_start(HexGloveHandle handle, const char* port_name);
```

| 输入参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `handle` | `HexGloveHandle` | 是 | `hex_glove_create` 返回的有效句柄。 |
| `port_name` | `const char*` | 是 | NUL 结尾的 UTF-8 串口名，例如 `COM3`、`/dev/ttyUSB0`。 |

**输出与返回值**：无输出参数；`1` 表示串口和启动命令成功，`0` 表示失败。成功后仍要等设备新帧。**示例**：`if (!hex_glove_start(glove, "COM3")) { /* 查询打开失败阶段 */ }`。**失败处理**：调用 `hex_glove_get_last_open_error` 读取阶段及 Windows／Linux 原始系统码；普通断开调用 `hex_glove_abort`。

### 读取接收器 ID：`hex_glove_get_device_id`

**接口描述**：获取 SDK 在当前串口连接中解析的六位 ASCII 接收器 ID。示例回复对应 `114205`。这是一个接收器 ID，不是左右手各自的序列号。

```c
int hex_glove_get_device_id(HexGloveHandle handle, char* output, uint32_t capacity);
```

| 参数 | 方向 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- | --- |
| `handle` | 输入 | `HexGloveHandle` | 是 | 已打开串口的会话。 |
| `output` | 输出 | `char*` | 是 | 可写缓冲区，成功时写入六位数字和末尾 NUL。 |
| `capacity` | 输入 | `uint32_t` | 是 | `output` 的**字节**容量，至少 `7`。 |

**返回值**：`1` 已收到并写入有效 ID；`0` 为回复未到、连接已断或参数错误。**示例**：

```cpp
char id[7]{};
if (hex_glove_get_device_id(glove, id, sizeof(id))) {
    std::printf("receiver_id=%s\n", id);
}
```

**失败处理**：该接口的 `0` 不是“设备无 ID”的最终结论。持续等待时可调用 `hex_glove_request_device_id(glove)` 重发指令，继续轮询；重发返回 `1` 只说明写入成功。断开和重连会清空旧 ID。

### 推进自动连接：`hex_partner_connect` 与 `hex_partner_tick`

**接口描述**：在应用线程启动连接事务，并持续推进端口搜索、接收器状态、新帧、超时、重连和标定。两函数配合使用。

```c
int hex_partner_connect(HexPartnerHandle handle, int64_t monotonic_ms);
int hex_partner_tick(HexPartnerHandle handle, int64_t monotonic_ms);
```

| 输入参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `handle` | `HexPartnerHandle` | 是 | 由 `hex_partner_create` 创建的会话。 |
| `monotonic_ms` | `int64_t` | 是 | 当前主机单调时钟毫秒；后续 `tick` 传入值不得倒退。 |

**输出与返回值**：`connect=1` 表示请求受理；`tick=1` 表示本次推进成功，**均不代表已经有可用骨骼**。读取 `HexPartnerStatus.phase`、`fresh_hand_mask` 和 `operation_state` 判断最终结果。**示例**：`hex_partner_connect(partner, now_ms());` 后按定时器调用 `hex_partner_tick(partner, now_ms());`。**失败处理**：读取 `status.last_error`、`operation_error` 和事件；使用 `hex_partner_error_string` 获取原因文本。

### 读取统一帧：`hex_external_get_frame`

**接口描述**：复制最近一次 `hex_external_update` 或 `hex_external_capture` 缓存的左右手 21 节点和 15 项关节角；读取本身不推进求解。

```c
int hex_external_get_frame(HexExternalHandle handle, HexExternalFrame* output);
```

| 参数 | 方向 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- | --- |
| `handle` | 输入 | `HexExternalHandle` | 是 | 由同一 glove／mocap 对创建的快照会话。 |
| `output` | 输出 | `HexExternalFrame*` | 是 | 调用前清零并设 `output->struct_size=sizeof(*output)`。 |

**返回值**：`1` 复制成功，`0` 失败。成功后先判断 `fresh_hand_mask`、`tracked_hand_mask`、`angle_valid_mask`，再使用 `pose[hand]` 和 `flexion_deg[hand][15]`。**示例**：

```cpp
HexExternalFrame frame{};
frame.struct_size = sizeof(frame);
if (hex_external_get_frame(external, &frame) &&
    (frame.fresh_hand_mask & frame.tracked_hand_mask & 1u)) {
    const auto& left = frame.pose[0];  // 左手 21 节点
    std::printf("left_seq=%u\n", left.frame_sequence);
}
```

**失败处理**：查 `hex_external_get_last_error`。`host_timestamp_ms` 是主机单调毫秒，不是 Unix UTC；双手来自同一宿主更新轮次，但硬件端不保证同一采样瞬间。

### 开始 R：`hex_mocap_start_r_capture`

**接口描述**：让选定手在指定时长内采集传感器 R 零位。使用 partner 时也可调用 `hex_partner_calibration_begin` 管理工作流。

```c
int hex_mocap_start_r_capture(HexMocapHandle handle, int hand_mask, float seconds);
```

| 输入参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `handle` | `HexMocapHandle` | 是 | 当前骨骼会话。 |
| `hand_mask` | `int` | 是 | `1` 左、`2` 右、`3` 双手；`3` 要求两手同时有有效数据。 |
| `seconds` | `float` | 是 | R 采样时长，单位秒；例如 `1.0f`。 |

**返回值**：`1` 采集已启动；`0` 未能启动。**示例**：`hex_mocap_start_r_capture(mocap, 3, 1.0f);` 后持续调用选定更新路径，轮询 `hex_mocap_get_r_capture_info` 的 `state/progress`。**失败处理**：确认所选手在线且有新帧，双手模式须两手同时满足。R 完成后在伸直中立位调用几何角度零位校准，二者不是同一步。

### 错误与结果从哪里读取

| 调用层 | 失败与最终结果 | 适用范围 |
| --- | --- | --- |
| 底层串口 | `hex_glove_get_last_open_error` 的 `stage/platform_code` | `hex_glove_start` 的最近一次打开失败；系统码须结合 Windows／Linux 解释。 |
| 高层工作流 | `HexPartnerStatus.last_error`、`operation_state/error`；`hex_partner_get_event` | 搜索、连接、重连、标定、档案操作。`hex_partner_error_string` 将代码转成文本。 |
| 统一快照 | `hex_external_get_last_error` 的 `HexExternalErrorInfo.code/message_utf8` | 仅覆盖 `hex_external_*` 包装调用；不自动记录任意旧接口失败。 |
| 数据有效性 | `fresh_hand_mask`、`tracked_hand_mask`、`angle_valid_mask` | 函数返回 `1` 时仍须判断；没有新帧不一定是函数错误。 |

`HexExternalErrorCode`：`0 OK`、`1 INVALID_ARGUMENT`、`2 BAD_STRUCT_SIZE`、`3 PORT_OPEN_FAILED`、`4 PROFILE_LOAD_FAILED`、`5 UPDATE_FAILED`、`6 NO_CAPTURED_FRAME`、`7 NO_TRACKED_HAND`、`8 ANGLE_CALIBRATION_FAILED`、`9 NO_FRESH_FRAME`。完整 `HexPartnerError` 数值以 `hex_partner_api.h` 为准。

## 2. 高层连接与标定：`hex_partner_api.h`

以下 `handle` 均为 `HexPartnerHandle`。`connect`、`tick`、标定和档案操作的异步最终结果见 `HexPartnerStatus.operation_sequence/kind/state/error`；`get_event` 可按序号读取事件。普通失败从 `status.last_error` 读取 `HexPartnerError`，再用 `hex_partner_error_string` 解释。

| 函数完整签名 | 参数说明 | 返回值／条件 |
| --- | --- | --- |
| `uint32_t hex_partner_get_api_version(void)` | 无参数。 | 工作流 ABI 版本号。 |
| `int hex_partner_config_init(HexPartnerConfig* config)` | `config`：调用方可写配置结构，随后可修改端口、超时等字段。 | `1/0`；创建 partner 前调用。 |
| `HexPartnerHandle hex_partner_create(HexGloveHandle glove, HexMocapHandle mocap, HexExternalHandle external, const HexPartnerConfig* config)` | `glove`：已创建且空闲的手套句柄；`mocap`：由此 glove 创建的骨骼句柄；`external`：同一对句柄创建的可选快照句柄，可为 `NULL`；`config`：经初始化的配置。 | 成功返回句柄，失败返回 `NULL`；不接管传入句柄所有权。 |
| `void hex_partner_destroy(HexPartnerHandle handle)` | `handle`：partner 句柄。 | 无返回；先于底层三个句柄销毁。 |
| `int hex_partner_connect(HexPartnerHandle handle, int64_t monotonic_ms)` | `monotonic_ms`：调用时的主机单调毫秒。 | `1` 表示连接请求受理，**不表示串口或双手已就绪**；继续 `tick` 并查 `phase`、`fresh_hand_mask`。 |
| `int hex_partner_disconnect(HexPartnerHandle handle)` | `handle`：目标会话。 | `1/0`；停止自动重连并关闭串口。 |
| `int hex_partner_stream_control(HexPartnerHandle handle, int start)` | `start=1` 发开始流命令；`start=0` 发停止流命令。 | `1` 只表示串口写入成功，非硬件确认；开始后等待新帧。 |
| `int hex_partner_tick(HexPartnerHandle handle, int64_t monotonic_ms)` | `monotonic_ms`：非递减的主机单调毫秒；按应用循环持续传入。 | `1/0`；推进搜索、数据、断流、重连和标定。 |
| `int hex_partner_get_status(HexPartnerHandle handle, HexPartnerStatus* output)` | `output`：输出状态；先设 `output->struct_size=sizeof(*output)`。 | `1/0`；读阶段、在线/新帧位、端口、错误和最近操作结果。 |
| `int hex_partner_get_timing(HexPartnerHandle handle, HexPartnerTiming* output)` | `output`：先设 `struct_size`。 | `1/0`；`observed_hz[0/1]` 为主机观测新帧 Hz，非硬件采样率。 |
| `int hex_partner_get_event(HexPartnerHandle handle, uint32_t after_sequence, HexPartnerEvent* output)` | `after_sequence`：上次已处理事件的 `sequence`，初次用 `0`；`output`：事件输出，先设 `struct_size`。 | `1` 有更新事件；`2` 无新事件；`0` 参数错误。每次处理后保存新序号。 |
| `const char* hex_partner_error_string(uint32_t code)` | `code`：`HexPartnerError` 数值。 | 静态英文说明字符串；调用方不释放。 |
| `int hex_partner_calibration_begin(HexPartnerHandle handle, uint32_t hand_mask, uint32_t mode, float r_seconds)` | `hand_mask`：`1/2/3`；`mode`：`HEX_PARTNER_CALIBRATION_R=0` 或 `HEX_PARTNER_CALIBRATION_FULL=1`；`r_seconds`：R 采样秒数。 | `1` 开始事务；持续 `tick` 并用 `get_calibration` 查看最终结果。FULL 是现有引导标定流程，具体七步 UI 由上位机按提示实现。 |
| `int hex_partner_calibration_advance(HexPartnerHandle handle, int hand)` | `hand`：当前完成动作的手 `0/1`。 | `1/0`；按 `prompt_utf8[hand]` 完成姿态后推进 FULL 流程。 |
| `int hex_partner_calibration_commit_ring(HexPartnerHandle handle, int hand, const float* measured_mm4, const float* model_mm4, float palm_measured, float palm_model)` | `hand`：`0/1`；`measured_mm4`：四指实测环长数组，单位毫米；`model_mm4`：四指模型参考环长数组，单位毫米；`palm_measured`/`palm_model`：手掌实测/模型参考值，单位毫米。 | `1/0`；FULL 流程提示环长时调用；两个指针均须指向至少 4 个 `float`。 |
| `int hex_partner_calibration_cancel(HexPartnerHandle handle)` | `handle`：正在标定的会话。 | `1/0`；取消当前标定。 |
| `int hex_partner_calibration_retry(HexPartnerHandle handle)` | `handle`：上次失败的会话。 | `1/0`；重试最近标定事务。 |
| `int hex_partner_get_calibration(HexPartnerHandle handle, HexPartnerCalibrationStatus* output)` | `output`：先设 `struct_size`。 | `1/0`；查看模式、R 状态、双手步骤、进度、提示和最终结果。 |
| `int hex_partner_profile_save(HexPartnerHandle handle)` | `handle`：已选定接收器端口的会话。 | `1/0`；按端口与左右手槽位保存双手 `.hexprofile`。 |
| `int hex_partner_profile_load(HexPartnerHandle handle)` | `handle`：已选定接收器端口的会话。 | `1/0`；加载对应档案。端口绑定**不是物理设备唯一 ID**。 |
| `int hex_partner_profile_path(HexPartnerHandle handle, char* output, uint32_t capacity, uint32_t* required)` | `output`：UTF-8 输出缓冲区；`capacity`：缓冲区字节数；`required`：所需字节数输出。 | `1/0`；先查询 `required` 并按返回长度分配，保留结尾 NUL。 |
| `uint32_t hex_partner_encode_legacy_udp(HexPartnerHandle handle, uint32_t channel, int64_t utc_ms, char* output, uint32_t capacity)` | `channel`：`0` 角度、`1` 姿态、`2` 位置、`3` 旋转；`utc_ms`：调用方提供 UTC Unix 毫秒；`output/capacity`：JSON 缓冲区及容量。 | 成功返回包括 NUL 在内所需字节数，失败 `0`；可用 `output=NULL,capacity=0` 查询大小；应在有新帧的 `tick` 后调用。 |

`HexPartnerConfig`：`auto_detect/auto_reconnect/auto_load_profile/auto_save_profile` 为开关；`probe_ms/reconnect_ms/data_timeout_ms` 为毫秒；`preferred_port_utf8[256]` 可指定端口；`profile_dir_utf8[512]` 指定档案目录。先调用 `config_init`，仅修改需要覆盖的项。`HexPartnerStatus.device_code[2][3]` 是短编码，不能当设备唯一序列号；`port_error_stage/platform_error_code` 提供串口失败阶段和 Windows `GetLastError` 或 Linux `errno` 原始码。

## 3. 统一骨骼与角度快照：`hex_external_api.h`

以下 `handle` 均为 `HexExternalHandle`。这些包装调用的错误由 `hex_external_get_last_error` 查询；它不会自动诊断任意旧版 API 的错误。

| 函数完整签名 | 参数说明 | 返回值／条件 |
| --- | --- | --- |
| `HexExternalHandle hex_external_create(HexGloveHandle glove, HexMocapHandle mocap)` | `glove`：手套句柄；`mocap`：由同一 glove 创建的骨骼句柄。 | 成功返回会话，失败 `NULL`；借用两者。 |
| `void hex_external_destroy(HexExternalHandle handle)` | `handle`：外部快照会话。 | 无返回；不会销毁 glove/mocap。 |
| `uint32_t hex_external_get_api_version(void)` | 无参数。 | 外部快照接口 ABI 版本号。 |
| `int hex_external_start(HexExternalHandle handle, const char* port_utf8)` | `port_utf8`：串口名。 | `1/0`；打开串口并启动接收。 |
| `int hex_external_profile_load(HexExternalHandle handle, const char* path_utf8)` | `path_utf8`：`.hexprofile` 文件路径。 | `1/0`；加载后清除旧快照和角度零位。 |
| `int hex_external_reset(HexExternalHandle handle)` | `handle`：外部快照会话。 | `1/0`；外部自行重连后清除缓存帧及角度零位。 |
| `int hex_external_update(HexExternalHandle handle)` | `handle`：外部快照会话。 | `1/0`；推进一次 mocap 并捕获同一轮左右手最终姿态/角度。 |
| `int hex_external_capture(HexExternalHandle handle)` | `handle`：外部快照会话。 | `1/0`；应用已经自行 `hex_mocap_update` 时只缓存结果，**不再推进**。 |
| `int hex_external_get_frame(HexExternalHandle handle, HexExternalFrame* output)` | `output`：先设 `struct_size=sizeof(*output)`。 | `1/0`；复制最近快照，不推进求解；先检查左右手掩码。 |
| `int hex_external_calibrate_angles(HexExternalHandle handle, uint32_t hand_mask)` | `hand_mask`：`1/2/3`，选定已跟踪且保持伸直中立的手。 | `1/0`；R 后建立 Hub 与精确角度零位。 |
| `int hex_external_get_last_error(HexExternalHandle handle, HexExternalErrorInfo* output)` | `output`：先设 `struct_size`。 | `1/0`；`code` 为 `HexExternalErrorCode`，`message_utf8` 为解释文本。 |
| `const char* hex_external_error_string(uint32_t code)` | `code`：`HexExternalErrorCode`。 | 静态英文字符串，不释放。 |

`HexExternalFrame.pose[0/1]` 各有 21 节点；`flexion_deg[0/1][15]` 对应左右手的 `thumb_cmc, thumb_mcp, thumb_ip, index_mcp, index_pip, index_dip, middle_mcp, middle_pip, middle_dip, ring_mcp, ring_pip, ring_dip, little_mcp, little_pip, little_dip`，单位度。`tracked_hand_mask` 表示有真实跟踪骨骼，`fresh_hand_mask` 表示本轮有新数据，`angle_valid_mask` 表示 15 项角度有效。`update_index` 是捕获次数，**不是设备帧号**；`host_timestamp_ms` 是单调时钟，**不是 UTC**。两手在同一宿主更新后缓存，不保证硬件同步采样。

## 4. 直接连接与设备信息：`hex_glove_api.h`、`hex_hardware_info_api.h`

以下 `handle` 均为 `HexGloveHandle`。使用 partner 时不要直接调用本节的连接/断开函数。

| 函数完整签名 | 参数说明 | 返回值／条件 |
| --- | --- | --- |
| `HexGloveHandle hex_glove_create(void)` | 无参数。 | 成功返回会话，失败 `NULL`；尚未打开串口。 |
| `void hex_glove_destroy(HexGloveHandle handle)` | `handle`：会话。 | 无返回；应在销毁 mocap 后调用。 |
| `int hex_glove_start(HexGloveHandle handle, const char* port_name)` | `port_name`：UTF-8 串口名。 | `1/0`；失败后查 `hex_glove_get_last_open_error`。 |
| `int hex_glove_get_last_open_error(HexGloveHandle handle, uint32_t* stage, uint32_t* platform_code)` | `stage`：失败阶段输出（`0` 无、`1` 端口名、`2` 系统打开、`3` 串口配置、`4` 启动命令）；`platform_code`：原始系统错误码输出。 | `1/0`；两输出指针必须有效。 |
| `void hex_glove_abort(HexGloveHandle handle)` | `handle`：已启动会话。 | 无返回；正常断开并关闭串口。 |
| `int hex_glove_get_device_snapshot(HexGloveHandle handle, HexGloveDeviceSnapshot* out)` | `out`：设备基础信息输出。 | `1/0`；仍检查左右在线、信道回复和硬件字节有效位。此结构没有 `struct_size`。 |
| `int hex_glove_get_hardware_info(HexGloveHandle handle, int hand, HexGloveHardwareInfo* info)` | `hand`：`0/1`；`info`：先设 `struct_size=sizeof(*info)`。 | `1/0`；并检查 `info->valid`，再使用固件、硬件及传感器字段。 |
| `int hex_glove_request_device_id(HexGloveHandle handle)` | `handle`：已打开串口的手套会话。 | `1` 表示重新发送读 ID 指令，非已收到回复；启动时 SDK 已自动发送一次。 |
| `int hex_glove_get_device_id(HexGloveHandle handle, char* output, uint32_t capacity)` | `output`：至少 7 字节的可写缓冲区；`capacity`：字节容量，须 `>=7`。 | `1` 写入六位 ASCII 接收器 ID 及 `\0`；`0` 表示尚无有效回复或参数错误。断开后旧 ID 清空。 |
| `void hex_glove_send_get_route(HexGloveHandle handle)` | `handle`：已连接会话。 | 无返回；请求当前无线信道，连接后请求一次即可。 |
| `int hex_glove_is_route_received(HexGloveHandle handle)` | `handle`：会话。 | 非零表示已收到信道回复。 |
| `int hex_glove_get_route(HexGloveHandle handle)` | `handle`：会话。 | 信道号；调用前先确认回复已到达。 |
| `uint64_t hex_glove_get_rx_bytes(HexGloveHandle handle)` | `handle`：会话。 | 累计串口接收字节数。 |
| `int hex_glove_get_receive_diagnostics(HexGloveHandle handle, HexGloveReceiveDiagnostics* out)` | `out`：先设 `struct_size`。 | `1/0`；累计字节、帧、校验不匹配、成帧错误和四类旋转包计数。 |

`HexGloveDeviceSnapshot` 的 `batt_left/right` 是电量估算；离线时不要显示为实时电量。`route_received=0` 时 `route` 未确认。`HexGloveHardwareInfo.software_major/minor` 为固件版本，`hardware_major/minor` 为硬件版本；`device_code` 为短编码，非唯一设备 ID。`hex_glove_shutdown` 是设备关机命令，普通断开用 `hex_glove_abort`。

接收器 ID 是另一条独立回复：命令 `A0 FF FF 28 01 05 FF 0A`，示例回复中的 `31 31 34 32 30 35` 为 ASCII `114205`。SDK 核验异或校验和六位数字后才让 `hex_glove_get_device_id` 返回成功。它标识当前接收器，不标识左/右手套；现有 profile 自动绑定仍使用端口和左右槽位。

## 5. 21 节点、R 与精确角度：`hex_mocap_api.h`

以下 `handle` 均为 `HexMocapHandle`；它由同一个 glove 创建。角度单位为度；伸直中立位角度校准后应接近 `0°`，但有符号角可能为负。

| 函数完整签名 | 参数说明 | 返回值／条件 |
| --- | --- | --- |
| `HexMocapHandle hex_mocap_create(HexGloveHandle glove)` | `glove`：已创建手套会话。 | 成功返回骨骼句柄，失败 `NULL`。 |
| `void hex_mocap_destroy(HexMocapHandle handle)` | `handle`：骨骼会话。 | 无返回；不销毁 glove。 |
| `int hex_mocap_update(HexMocapHandle handle)` | `handle`：骨骼会话。 | `1/0`；每轮推进一次，成功后仍检查新帧与跟踪位。 |
| `int hex_mocap_skeleton_definition_init(HexMocapSkeletonDefinition* definition, int hand)` | `definition`：骨架定义输出，先设 `struct_size`；`hand`：`0/1`。 | `1/0`；生成默认骨架，若需与自身模型一致可调整后上传。 |
| `int hex_mocap_set_skeleton_definition(HexMocapHandle handle, int hand, const HexMocapSkeletonDefinition* definition)` | `hand`：`0/1`；`definition`：21 节点父索引、静止局部变换与根变换。 | `1/0`；更换该手显示骨架。 |
| `int hex_mocap_get_display_pose(HexMocapHandle handle, int hand, HexMocapDisplayPose* pose)` | `hand`：`0/1`；`pose`：先设 `struct_size`。 | `1/0`；`valid_mask` 为模型节点，`tracked_mask` 才是实时跟踪节点。 |
| `int hex_mocap_profile_load(HexMocapHandle handle, const char* path_utf8)` | `path_utf8`：校准配置文件路径。 | `1/0`；宜在逐帧求解前加载。 |
| `int hex_mocap_start_r_capture(HexMocapHandle handle, int hand_mask, float seconds)` | `hand_mask`：`1/2/3`；`seconds`：采样时长（秒）。 | `1/0`；随后持续 `update` 并查询 R 进度；双手 mask 3 同时采样。 |
| `int hex_mocap_get_r_capture_info(HexMocapHandle handle, HexMocapRCaptureInfo* info)` | `info`：先设 `struct_size`。 | `1/0`；读取 `state`、`hand_mask`、`progress` 和新帧手位。 |
| `int hex_mocap_calibrate_angles(HexMocapHandle handle, int hand_mask)` | `hand_mask`：`1/2/3`，所选手须有有效姿态并保持伸直中立。 | `1/0`；在传感器 R 后建立**独立的几何角度零位**。 |
| `int hex_mocap_get_precise_angles(HexMocapHandle handle, int hand, HexAnglesSnapshot* angles)` | `hand`：`0/1`；`angles`：先设 `struct_size`。 | `1/0`；检查 `calibrated` 和 `valid_mask`。`finger[5][4]` 无独立拇指 CMC 项。 |
| `uint32_t hex_mocap_get_api_version(void)` | 无参数。 | 软件 API 版本号，非设备固件版本。 |
| `uint32_t hex_mocap_get_status(HexMocapHandle handle)` | `handle`：骨骼会话。 | 状态位集合；按 `mocap_abi.h` 的位掩码检查无设备、待数据、R 未完成等状态。 |
| `uint32_t hex_mocap_get_fresh_hand_mask(HexMocapHandle handle)` | `handle`：骨骼会话。 | 最近一次更新有新帧的手位 `0..3`。 |
| `uint32_t hex_mocap_get_capabilities(void)` | 无参数。 | `HEX_MOCAP_CAP_*` 能力位；不等同于所有内部功能都对外开放。 |
| `int hex_mocap_get_abi_info(HexMocapAbiInfo* info)` | `info`：先设 `struct_size`。 | `1/0`；核对指针大小、结构大小、偏移和 `display_joint_count=21`。 |

`HexMocapDisplayPose` 坐标为 Unity 约定：位置单位米；`local_position` 相对父节点，`world_position` 为世界位置。`pose.timestamp_ms` 为主机单调时间，`frame_sequence` 为该手帧序号。左右手采用相同节点顺序，但局部轴和拇指修正有差异。具体节点编号见 [对外 API 说明](EXTERNAL_API.zh-CN.md)。

## 6. 关节角 Hub：`hex_angles_api.h`

Hub 的配置和状态均由调用方持有；**每只手各维护一份 `HexAngleHubState`**。与快照角度相比，直接 Hub 路径适合需要自定义关节轴或校准逻辑的接入方。`camera_forward`、`nail_world_normal` 可以传 `NULL`，含义和输出有效性以头文件及算法状态为准。

| 函数完整签名 | 参数说明 | 返回值／条件 |
| --- | --- | --- |
| `int hex_angle_hub_config_init(HexAngleHubConfig* config)` | `config`：21 槽配置；先设 `struct_size`。 | `1/0`；初始化默认关节定义。 |
| `int hex_angle_hub_state_init(HexAngleHubState* state)` | `state`：调用方状态；先设 `struct_size`。 | `1/0`；初始化校准与角度缓存。 |
| `int hex_angle_hub_calibrate(const HexAngleHubConfig* config, HexAngleHubState* state, const HexMocapDisplayPose* pose, const HexMocapVec3* camera_forward)` | `config/state`：已初始化；`pose`：当前有效最终骨骼；`camera_forward`：可选相机前向向量，或 `NULL`。 | `1/0`；以当前姿态建立该手角度零位。 |
| `int hex_angle_hub_update(const HexAngleHubConfig* config, HexAngleHubState* state, const HexMocapDisplayPose* pose, float dt_seconds, const HexMocapVec3* camera_forward, const HexMocapVec3* nail_world_normal)` | `pose`：当前有效骨骼；`dt_seconds`：本次与上次调用间隔秒数；两个向量：可选相机方向/指甲法向，或 `NULL`。 | `1/0`；检查 `state.valid_mask` 后读取 `flexion[21]`/`abduction[21]`。 |

Hub 槽位：`1/2/3` 为拇指 CMC/MCP/IP；`5/6/7` 食指 MCP/PIP/DIP；`9/10/11` 中指；`13/14/15` 无名指；`17/18/19` 小指。伸直中立校准后接近 0°；不要将 Precise 和 Hub 角混写成同一算法字段。

## 7. 常见调用顺序与错误处理

1. 创建 `glove` → `mocap` → 可选 `external`，初始化配置并创建 `partner`。
2. `hex_partner_connect(partner, monotonic_ms)` 后定期 `hex_partner_tick`；查看 `phase`、`fresh_hand_mask`、`last_error`。连接受理不代表已有有效骨骼。
3. 加载 `.hexprofile`（可由 partner 自动加载）→ R 采样 → 在伸直中立位做角度零位校准 → 读取同一轮 `HexExternalFrame`。
4. 只在相应 `fresh_hand_mask`、`tracked_hand_mask`、`angle_valid_mask` 有效时输出实时数据；导出 UTC 字段由应用额外获取系统时间。
5. 停止时 `hex_partner_disconnect` → `hex_partner_destroy` → `hex_external_destroy`（若有）→ `hex_mocap_destroy` → `hex_glove_destroy`。

低层串口失败用 `hex_glove_get_last_open_error`，高层工作流失败用 `HexPartnerStatus.last_error`、`operation_error` 和事件；快照包装失败用 `hex_external_get_last_error`。这些错误渠道各自覆盖自己的接口，不能相互替代。CSV/JSON/实时流由接入程序基于 SDK 帧自行序列化，参考 [对外接入指南](EXTERNAL_INTEGRATION.zh-CN.md)。
