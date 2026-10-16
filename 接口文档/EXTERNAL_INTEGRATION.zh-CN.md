# HexaCercle 手套 SDK｜接入与调用指南

这份指南给首次接入的开发者使用，按“连接 → 加载校准 → 查询设备 → R → 读取双手 21 节点及关节角 → 导出数据 → 断开”的顺序说明。对外开放的函数与字段见 [API 说明](EXTERNAL_API.zh-CN.md)。

> **适用范围**：Windows x64／Linux x64；C++17、Qt 6.5+ 或 Python `ctypes`  
> **交付文件**：`library/include/hex_glove/`、`library/windows-x64/`、`library/linux-x64/`

[文档入口](README.zh-CN.md) · [API 概览](EXTERNAL_API.zh-CN.md) · [函数参数手册](EXTERNAL_API_PARAMETERS.zh-CN.md) · [上位机工作流](PARTNER_INTEGRATION.zh-CN.md)

## 本页目录

1. [接入前准备](#接入前准备)
2. [统一快照调用顺序](#推荐的快照调用顺序)
3. [手动连接调用顺序](#手动连接调用顺序)
4. [C++ 最小示例](#c-最小示例)
5. [R 校准](#r-校准调用片段)
6. [时间戳和导出](#关节角和带时间戳的数据导出)
7. [示例入口](#可运行示例)

需要 SDK 自动搜索/重连、连接状态事件和校准文件自动加载时，使用[上位机对接指南](PARTNER_INTEGRATION.zh-CN.md)的 `hex_partner_*` 流程；本页以下基础流程仍适用于已知串口且由应用自行管理连接的情况。

## 接入前准备

1. 使用与当前编译产物配套的 `library/include/hex_glove/` 头文件与原生库：Windows 为 `HexaCercleGlove.dll`，Linux 为 `libHexaCercleGlove.so`。
2. 准备串口名称，如 `COM3` 或 `/dev/ttyUSB0`。确保串口没有被其他上位机占用。
   打开串口后 SDK 自动发送读 ID 指令。用 `hex_glove_get_device_id(glove, id, sizeof(id))` 等待六位接收器 ID；回复未到时返回 `0`。完整签名见[函数参数手册](EXTERNAL_API_PARAMETERS.zh-CN.md)。
3. 如需复现已校准的手型和姿态，准备由同版本 SDK 保存的 `.hexprofile` 文件。它包含模型与校准数据；没有该文件时只能先使用演示合成骨架验证链路。
4. 运行程序前确保系统能找到原生库及其依赖。Python、应用程序和 DLL 的架构必须一致。

推荐先阅读 [统一快照 C++ 示例](../examples/external_snapshot/main.cpp) 和 [统一快照 Python 示例](../examples/external_snapshot/main.py)。它们使用同一次更新缓存的左右手骨骼及 Hub 角度，并演示状态、接收诊断、ABI、错误码、CSV 和 JSON Lines。原有快速入门示例仍展示逐个底层接口的调用方式。

## 推荐的快照调用顺序

```text
检查 hex_mocap_get_abi_info 与 hex_mocap_get_capabilities
hex_glove_create → hex_mocap_create → hex_external_create
hex_external_profile_load（有配置文件时）→ hex_external_start
循环：hex_external_update → hex_external_get_frame
      检查 tracked_hand_mask、fresh_hand_mask、angle_valid_mask
      查询 hex_mocap_get_status 与 hex_glove_get_receive_diagnostics
      需要 R 时启动 R、继续 update、等待 R 完成
      保持伸直中立 → hex_external_calibrate_angles → 后续新帧导出角度
hex_external_destroy → hex_mocap_destroy → hex_glove_abort → hex_glove_destroy
```

`hex_external_get_frame` 只复制缓存，不会让骨骼与角度来自不同的两次更新。宿主时间戳为单调时间；如需 CSV/JSON 对应公历时间，应用应另加 UTC 毫秒字段。若 `update` 失败，用 `hex_external_get_last_error` 查包装接口错误，再结合状态位和接收计数器定位。`HEX_EXTERNAL_PORT_OPEN_FAILED` 是包装层的概括错误；需要排查串口时再调用 `hex_glove_get_last_open_error` 读取打开阶段和系统原始错误码。

## 手动连接调用顺序

```text
hex_glove_create
  → hex_mocap_create(glove)
  → hex_mocap_profile_load(mocap, profilePath)    // 有校准文件时
  → hex_glove_start(glove, port)
  → 轮询 hex_glove_get_device_id(glove, id, 7) // 串口打开后 SDK 自动请求 ID
  → hex_glove_send_get_route(glove)               // 连接后请求一次
  → 循环：hex_mocap_update(mocap)
           hex_glove_get_device_snapshot(glove, ...)
           hex_glove_get_hardware_info(glove, hand, ...)
           hex_mocap_get_display_pose(mocap, hand, ...)
  → 需要重新 R 时：hex_mocap_start_r_capture(mocap, mask, seconds)
           继续逐帧 update，并查询 hex_mocap_get_r_capture_info
  → hex_mocap_destroy
  → hex_glove_abort
  → hex_glove_destroy
```

先加载 profile，再开始逐帧读取；不要在加载后重新上传演示骨架。R 需要连接后有新鲜的有效手部数据，调用 `start_r_capture` 后仍要持续 `update`。普通 R 可对齐手掌朝向；重戴导致传感器安装零位变化时，应使用匹配当前佩戴状态的校准档案。常规断开使用 `abort`；`shutdown` 是设备关机命令。

## C++ 最小示例

以下代码展示 API 调用顺序和关键有效性判断。`profile.hexprofile` 换成自己的校准文件；如果暂无文件，先运行 [现有链路演示](../examples/sdk_quickstart/main.cpp) 的合成骨架流程。

```cpp
#include "hex_glove/hex_glove_api.h"
#include "hex_glove/hex_mocap_api.h"
#include <cstdio>

int run(const char* port, const char* profile_path) {
    HexGloveHandle glove = hex_glove_create();
    if (!glove) return 1;
    HexMocapHandle mocap = hex_mocap_create(glove);
    if (!mocap) { hex_glove_destroy(glove); return 2; }
    auto finish = [&](int code) {
        hex_mocap_destroy(mocap);
        hex_glove_abort(glove);
        hex_glove_destroy(glove);
        return code;
    };
    if (!hex_mocap_profile_load(mocap, profile_path)) return finish(3);
    if (!hex_glove_start(glove, port)) return finish(4);
    hex_glove_send_get_route(glove);
    std::printf("SDK API version: 0x%08x\n", hex_mocap_get_api_version());

    // 宿主在定时器/帧循环中重复调用以下代码。
    if (!hex_mocap_update(mocap)) return finish(5);
    HexGloveDeviceSnapshot device{};
    if (hex_glove_get_device_snapshot(glove, &device)) {
        std::printf("L=%d batt=%.0f%%  R=%d batt=%.0f%%\n",
            device.left_enabled, device.batt_left,
            device.right_enabled, device.batt_right);
        if (device.route_received) std::printf("route=%d\n", device.route);
        else std::puts("route=pending");
    }
    for (int hand = 0; hand < 2; ++hand) {
        HexGloveHardwareInfo hardware{};
        hardware.struct_size = sizeof(hardware);
        if (hex_glove_get_hardware_info(glove, hand, &hardware) && hardware.valid)
            std::printf("hand=%d firmware=%u.%u hardware=%u.%u\n", hand,
                hardware.software_major, hardware.software_minor,
                hardware.hardware_major, hardware.hardware_minor);

        HexMocapDisplayPose pose{};
        pose.struct_size = sizeof(pose);
        if (hex_mocap_get_display_pose(mocap, hand, &pose)) {
            for (int joint = 0; joint < 21; ++joint) {
                if (!(pose.tracked_mask & (1u << joint))) continue;
                const auto& p = pose.world_position[joint];
                const auto& q = pose.world_rotation[joint];
                std::printf("hand=%d joint=%d pos=(%.3f %.3f %.3f)m quat=(%.3f %.3f %.3f %.3f)\n",
                    hand, joint, p.x, p.y, p.z, q.x, q.y, q.z, q.w);
            }
        }
    }
    return finish(0);
}
```

真实程序应持续运行帧循环，上例只演示一次读取。设备刚打开时可能尚无数据；不要把首次查询的空结果当作永久连接失败。显示时按 `tracked_mask` 绘制节点，父子关系来自加载的骨架定义；二维线框示例见 [Qt 查看器](../examples/qt_skeleton_viewer/main.cpp)。

## R 校准调用片段

当应用需要重新设定佩戴零位时，在已经持续接收有效帧的状态下：

```cpp
int mask = (device.left_enabled ? 1 : 0) | (device.right_enabled ? 2 : 0);
if (mask && hex_mocap_start_r_capture(mocap, mask, 1.0f)) {
    // 后续每帧继续 hex_mocap_update(mocap)。
    HexMocapRCaptureInfo r{};
    r.struct_size = sizeof(r);
    if (hex_mocap_get_r_capture_info(mocap, &r)) {
        // r.state: 1 采样中、2 完成、3 失败、4 取消；r.progress 为进度。
    }
}
```

`mask` 和 `device` 在这个片段中来自本页上方的设备快照。R 完成后再以新的跟踪结果驱动显示；无需在宿主自行再叠加一层 R 偏移。

## 关节角和带时间戳的数据导出

精确角数组 `HexAnglesSnapshot.finger[5][4]` 使用度，行是拇指到小指，列为 MCP/PIP/DIP/spread；其中**不含单独的拇指 CMC**。本页示例要求按 `thumb_mcp`、`thumb_ip`、四指 MCP/PIP/DIP 逐项输出，因此使用同一套 JointAngleHub 的 `flexion[21]`，拇指 CMC 可额外输出。Hub 的拇指 IP 是第 3 槽，精确数组的拇指 PIP/DIP 不应直接重命名为 IP。

完成传感器 R 后，让手指伸直并保持中立姿势，使用当前有效的最终骨骼姿态为**每只手分别**调用 `hex_angle_hub_calibrate`；之后每帧调用 `hex_angle_hub_update`。这个几何角零位与传感器 R 不同。校准时若手指弯曲，那一姿势会被记作零位；伸直中立位校准后应接近 `0°`，但不能假设所有屈伸角都非负。左右手的顺序和 Unity 坐标约定相同，局部轴、镜像和拇指参数仍可能不同。

```cpp
#include "hex_glove/hex_angles_api.h"
#include <cstdio>

// 初始化一次；左右手各有独立状态。
HexAngleHubConfig cfg[2]{};
HexAngleHubState hub[2]{};
bool calibrated[2] = {false, false};
bool neutral_pose_confirmed[2] = {false, false}; // 由界面在用户摆好伸直中立姿势后置 true
for (int h = 0; h < 2; ++h) {
    cfg[h].struct_size = sizeof(cfg[h]);
    hub[h].struct_size = sizeof(hub[h]);
    hex_angle_hub_config_init(&cfg[h]);
    hex_angle_hub_state_init(&hub[h]);
}

// 以下代码在帧循环中执行；calibrated[h] 初始为 false。
// 每次完成新的 R 后，把相应的 calibrated[h] 重新设为 false。
for (int h = 0; h < 2; ++h) {
    HexMocapDisplayPose pose{};
    pose.struct_size = sizeof(pose);
    if (!hex_mocap_get_display_pose(mocap, h, &pose) || !pose.tracked_mask) continue;
    if (!calibrated[h]) {
        if (!neutral_pose_confirmed[h]) continue;
        calibrated[h] = hex_angle_hub_calibrate(&cfg[h], &hub[h], &pose, nullptr) != 0;
        continue;
    }
    if (!hex_angle_hub_update(&cfg[h], &hub[h], &pose, dt_seconds, nullptr, nullptr)) continue;
    const int slots[] = {1,2,3,5,6,7,9,10,11,13,14,15,17,18,19};
    bool all_valid = true;
    for (int j : slots)
        if (!(pose.tracked_mask & (1u << j)) || !(hub[h].valid_mask & (1u << j))) all_valid = false;
    if (!all_valid) continue;
    // CSV 文件先写下方表头；每个有效手、每个新帧写一行。
    std::fprintf(csv, "%lld,%lld,%u,%s", (long long)pose.timestamp_ms,
        (long long)utc_timestamp_ms, pose.frame_sequence, h == 0 ? "left" : "right");
    for (int j : slots) std::fprintf(csv, ",%.6f", hub[h].flexion[j]);
    std::fputc('\n', csv);

    // 同一帧也可写入 JSON Lines；jsonl 可为文件或标准输出(stdout)。
    const char* names[] = {"thumb_cmc","thumb_mcp","thumb_ip",
        "index_mcp","index_pip","index_dip","middle_mcp","middle_pip","middle_dip",
        "ring_mcp","ring_pip","ring_dip","little_mcp","little_pip","little_dip"};
    std::fprintf(jsonl, "{\"timestamp_ms\":%lld,\"utc_timestamp_ms\":%lld,\"frame_sequence\":%u,\"hand\":\"%s\"",
        (long long)pose.timestamp_ms, (long long)utc_timestamp_ms,
        pose.frame_sequence, h == 0 ? "left" : "right");
    for (int k = 0; k < 15; ++k)
        std::fprintf(jsonl, ",\"%s\":%.6f", names[k], hub[h].flexion[slots[k]]);
    std::fputs("}\n", jsonl);
    std::fflush(jsonl); // 管道或标准输出模式下逐帧交给下游
}
```

CSV 表头：

```csv
timestamp_ms,utc_timestamp_ms,frame_sequence,hand,thumb_cmc,thumb_mcp,thumb_ip,index_mcp,index_pip,index_dip,middle_mcp,middle_pip,middle_dip,ring_mcp,ring_pip,ring_dip,little_mcp,little_pip,little_dip
```

`dt_seconds` 是相邻有效帧间隔，`utc_timestamp_ms` 由宿主时钟提供；`pose.timestamp_ms` 是 SDK 的单调时间，不能直接当 UTC。`csv` 与 `jsonl` 是接入程序打开的 `FILE*`；使用 `jsonl=stdout` 时可通过管道实时消费每行完整 JSON。若只需要题述字段，可从表头、`slots` 和 `names` 中同时去掉 `thumb_cmc`/槽位 `1`。实时数据流也可将每行 JSON 发送到应用自己的 WebSocket、TCP 或 UDP 通道；只在新帧且有效位齐全时发送。CSV、JSON 和传输层由接入应用写出，SDK 提供实时姿态、时间戳和角度。输出字段顺序与角度含义见 [API 说明](EXTERNAL_API.zh-CN.md)。

若使用 `hex_mocap_get_precise_angles` 取得 `finger[5][4]`，需在有效跟踪后另行调用 `hex_mocap_calibrate_angles` 建立精确角零位。不要将这套掌局部角与 Hub 的拇指 CMC/MCP 混作同一算法口径。

## 数据解释速查

| 字段 | 接入方应如何使用 |
| --- | --- |
| `device.left_enabled/right_enabled` | 判断左右手当前是否在线。 |
| `device.batt_left/batt_right` | 在线时显示左右电量估算。 |
| `device.route_received` + `device.route` | 已收到路由回复后显示当前信道。 |
| `hardware.software_major/minor` | 固件版本；与 SDK API 版本分开显示。 |
| `hardware.hardware_major/minor` | 硬件版本；检查 `hardware.valid`。 |
| `pose.valid_mask` | 骨架模型中有哪些节点。 |
| `pose.tracked_mask` | 当前哪些节点有实时跟踪结果。 |
| `pose.world_position/rotation[21]` | 用于世界空间显示或传输；位置为米，旋转为 `(x,y,z,w)`。 |
| `pose.local_position/rotation/scale[21]` | 供具有相同骨架父子关系的模型应用局部变换。 |

## 可运行示例

- [Python / C++ / Qt 示例及构建命令](../examples/README.md)：适合先检查连接、数据读取和线框显示；其中未加载校准文件的示例使用合成骨架。


> 库版本：Windows 示例使用 当前源码构建；随包 Linux `.so` 已按 2026-10-15 从当前源码重新构建，需在目标 Linux 环境验证。




