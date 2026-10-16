#include "hex_glove/hex_glove_api.h"
#include "hex_glove/hex_mocap_api.h"
#include "hex_glove/hex_hardware_info_api.h"
#include "../angle_output.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

int main(int argc, char** argv) {
    // 端口是必需参数；profile、重新 R 和两种数据输出均为可选。
    if (argc < 2) { std::fprintf(stderr, "Usage: sdk_quickstart PORT [--profile FILE] [--r] [--csv FILE] [--jsonl FILE|-]\n"); return 1; }
    const char* profile = nullptr;
    const char* csv_path = nullptr;
    const char* jsonl_path = nullptr;
    bool request_r = false;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--profile") == 0 && i + 1 < argc) profile = argv[++i];
        else if (std::strcmp(argv[i], "--r") == 0) request_r = true;
        else if (std::strcmp(argv[i], "--csv") == 0 && i + 1 < argc) csv_path = argv[++i];
        else if (std::strcmp(argv[i], "--jsonl") == 0 && i + 1 < argc) jsonl_path = argv[++i];
        else { std::fprintf(stderr, "Unknown option: %s\n", argv[i]); return 1; }
    }
    AngleOutput angles;
    // 输出文件先于设备打开，避免运行中才发现路径不可写。
    if (!angles.open(csv_path, jsonl_path)) { std::fprintf(stderr, "Cannot open angle output file\n"); return 1; }
    std::fprintf(stderr, "Keep fingers straight and neutral until angle zero is captured after R.\n");
    HexGloveHandle glove = hex_glove_create();
    // glove 管理串口/接收；mocap 使用该连接解算骨骼和角度。
    if (!glove) return 2;
    HexMocapHandle mocap = hex_mocap_create(glove);
    if (!mocap) { hex_glove_destroy(glove); return 3; }
    if (profile && !hex_mocap_profile_load(mocap, profile)) {
        // 校准档案包含模型与已有校准数据，必须在逐帧求解前加载。
        std::fprintf(stderr, "Cannot load profile: %s\n", profile);
        hex_mocap_destroy(mocap); hex_glove_destroy(glove); return 4;
    }
    if (!hex_glove_start(glove, argv[1])) {
        // start 打开串口；正常断开时用 abort，而不是设备关机命令。
        std::fprintf(stderr, "Cannot open glove on %s\n", argv[1]);
        hex_mocap_destroy(mocap); hex_glove_destroy(glove); return 5;
    }
    // Synthetic geometry is for a visible diagnostic skeleton. Production
    // applications should upload their actual model rest pose instead.
    for (int hand = 0; !profile && hand < 2; ++hand) {
        HexMocapSkeletonDefinition model{};
        model.struct_size = sizeof(model);
        if (hex_mocap_skeleton_definition_init(&model, hand))
            hex_mocap_set_skeleton_definition(mocap, hand, &model);
    }
    // start 已发送读 ID；先等异步回复，再查询信道，避免两个指令紧挨着发送。
    bool route_requested = false;
    bool r_started = false;
    bool device_id_shown = false;
    for (int tick = 0; tick < 300; ++tick) { // ~30 seconds
        if (!device_id_shown) {
            char device_id[7]{};
            if (hex_glove_get_device_id(glove, device_id, sizeof(device_id))) {
                std::fprintf(stderr, "receiver_device_id=%s\n", device_id);
                device_id_shown = true;
            }
        }
        if (!device_id_shown && (tick == 5 || tick == 30))
            hex_glove_request_device_id(glove); // 首次/信道查询后各补发一次。
        if (!route_requested && (device_id_shown || tick >= 20)) {
            hex_glove_send_get_route(glove);
            route_requested = true;
        }
        HexGloveDeviceSnapshot device{};
        hex_glove_get_device_snapshot(glove, &device);
        const int mask = (device.left_enabled ? 1 : 0) | (device.right_enabled ? 2 : 0);
        hex_mocap_update(mocap);
        // 每帧推进原始输入、R 采样和最终 21 节点骨骼求解。
        if (!r_started && mask && (!profile || request_r)) {
            r_started = hex_mocap_start_r_capture(mocap, mask, 1.0f) != 0;
            std::fprintf(stderr, "R capture: %s\n", r_started ? "started" : "waiting for valid input");
        }
        HexMocapRCaptureInfo capture{};
        capture.struct_size = sizeof(capture);
        hex_mocap_get_r_capture_info(mocap, &capture);
        // R 与精确角/Hub 几何零位是不同校准；等待 R 完成再采角度零位。
        const bool angle_ready = (profile && !request_r) || capture.state == 2;
        for (int hand = 0; hand < 2; ++hand) {
            HexMocapDisplayPose pose{};
            pose.struct_size = sizeof(pose);
            if (!hex_mocap_get_display_pose(mocap, hand, &pose) || !pose.tracked_mask) continue;
            if (angle_ready && !angles.calibrated[hand]) {
                if (angles.calibrate(hand, pose)) {
                    // 同一姿态也用于 mocap 内置的 finger[5][4] 精确角度零位。
                    hex_mocap_calibrate_angles(mocap, 1 << hand);
                    std::fprintf(stderr, "%s angle zero captured at SDK timestamp %lld ms\n",
                        hand ? "right" : "left", static_cast<long long>(pose.timestamp_ms));
                }
            } else if (angle_ready) angles.update(hand, pose);
            // update 成功后由 AngleOutput 写带 SDK/UTC 时间戳的 CSV/JSONL。
        }
        if (tick % 10 == 0) {
            std::fprintf(stderr, "route=%d received=%d rx=%llu  L=%d/M%d batt=%.0f  R=%d/M%d batt=%.0f\n",
                device.route, device.route_received,
                static_cast<unsigned long long>(hex_glove_get_rx_bytes(glove)),
                device.left_enabled, device.layout_left, device.batt_left,
                device.right_enabled, device.layout_right, device.batt_right);
            for (int hand = 0; hand < 2; ++hand) {
                HexMocapDisplayPose pose{};
                pose.struct_size = sizeof(pose);
                if (hex_mocap_get_display_pose(mocap, hand, &pose)) {
                    std::fprintf(stderr, "  %c skeleton: defined=%08x tracked=%08x palm=(%.3f, %.3f, %.3f)m\n",
                        hand ? 'R' : 'L', pose.valid_mask, pose.tracked_mask,
                        pose.world_position[0].x, pose.world_position[0].y, pose.world_position[0].z);
                    if (angles.calibrated[hand]) {
                        std::fprintf(stderr, "    angles(deg): thumb_mcp=%.1f thumb_ip=%.1f index_mcp=%.1f index_pip=%.1f index_dip=%.1f\n",
                            angles.state[hand].flexion[2], angles.state[hand].flexion[3],
                            angles.state[hand].flexion[5], angles.state[hand].flexion[6], angles.state[hand].flexion[7]);
                        HexAnglesSnapshot precise{};
                        precise.struct_size = sizeof(precise);
                        if (hex_mocap_get_precise_angles(mocap, hand, &precise) && precise.calibrated)
                            // 精确角是另一套算法；CMC 须从 Hub 槽 1 读取。
                            std::fprintf(stderr, "    precise t=%lldms thumb_mcp=%.1f index_mcp=%.1f (deg; CMC is in Hub)\n",
                                static_cast<long long>(precise.timestamp_ms),
                                precise.finger[0][0], precise.finger[1][0]);
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    hex_mocap_destroy(mocap);
    // 先释放骨骼会话，再关闭串口，最后释放手套会话。
    hex_glove_abort(glove); // disconnect; shutdown would power off the device
    hex_glove_destroy(glove);
}
