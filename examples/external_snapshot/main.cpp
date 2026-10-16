#include "hex_glove/hex_external_api.h"
#include "hex_glove/hex_hardware_info_api.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

static constexpr const char* kNames[HEX_EXTERNAL_ANGLE_COUNT] = {
    "thumb_cmc","thumb_mcp","thumb_ip","index_mcp","index_pip","index_dip",
    "middle_mcp","middle_pip","middle_dip","ring_mcp","ring_pip","ring_dip",
    "little_mcp","little_pip","little_dip"};

static void print_error(HexExternalHandle external) {
    HexExternalErrorInfo error{};
    error.struct_size = sizeof(error);
    if (hex_external_get_last_error(external, &error))
        std::fprintf(stderr, "SDK error %u: %s\n", error.code, error.message_utf8);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: external_snapshot PORT [--profile FILE] [--r] [--csv FILE] [--jsonl FILE|-]\n");
        return 1;
    }
    const char* profile = nullptr;
    const char* csv_path = nullptr;
    const char* json_path = nullptr;
    bool request_r = false;
    for (int i = 2; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--profile") && i + 1 < argc) profile = argv[++i];
        else if (!std::strcmp(argv[i], "--r")) request_r = true;
        else if (!std::strcmp(argv[i], "--csv") && i + 1 < argc) csv_path = argv[++i];
        else if (!std::strcmp(argv[i], "--jsonl") && i + 1 < argc) json_path = argv[++i];
        else return 1;
    }
    std::fprintf(stderr, "API 0x%08x, external API 0x%08x, capabilities 0x%08x\n",
        hex_mocap_get_api_version(), hex_external_get_api_version(), hex_mocap_get_capabilities());
    HexMocapAbiInfo abi{};
    abi.struct_size = sizeof(abi);
    if (!hex_mocap_get_abi_info(&abi) || abi.pointer_size != sizeof(void*) || abi.display_joint_count != 21) {
        std::fprintf(stderr, "Incompatible SDK ABI\n"); return 2;
    }

    HexGloveHandle glove = hex_glove_create();
    HexMocapHandle mocap = glove ? hex_mocap_create(glove) : nullptr;
    HexExternalHandle external = mocap ? hex_external_create(glove, mocap) : nullptr;
    if (!external) { if (mocap) hex_mocap_destroy(mocap); if (glove) hex_glove_destroy(glove); return 3; }
    std::FILE* csv = csv_path ? std::fopen(csv_path, "w") : nullptr;
    std::FILE* jsonl = json_path ? (!std::strcmp(json_path, "-") ? stdout : std::fopen(json_path, "w")) : nullptr;
    int result = 0;
    if ((csv_path && !csv) || (json_path && !jsonl)) { std::fprintf(stderr, "Cannot open output file\n"); result = 4; goto cleanup; }
    if (csv) {
        std::fputs("timestamp_ms,utc_timestamp_ms,update_index,frame_sequence,hand", csv);
        for (const char* name : kNames) std::fprintf(csv, ",%s", name);
        std::fputc('\n', csv);
    }
    if (profile) {
        if (!hex_external_profile_load(external, profile)) { print_error(external); result = 5; goto cleanup; }
    } else {
        // Synthetic model is only a link/display demonstration.
        for (int h = 0; h < 2; ++h) {
            HexMocapSkeletonDefinition model{};
            model.struct_size = sizeof(model);
            if (hex_mocap_skeleton_definition_init(&model, h))
                hex_mocap_set_skeleton_definition(mocap, h, &model);
        }
    }
    if (!hex_external_start(external, argv[1])) { print_error(external); result = 6; goto cleanup; }
    // SDK 启动时已请求接收器 ID；等回复后再发信道查询。
    std::fprintf(stderr, "Hold fingers straight and neutral until angle calibration finishes.\n");
    {
        bool r_started = false;
        bool route_requested = false;
        bool device_id_shown = false;
        char receiver_id_cache[7]{};
        uint32_t calibrated_mask = 0;
        for (int tick = 0; tick < 300; ++tick) {
            if (!device_id_shown) {
                char receiver_id[7]{};
                if (hex_glove_get_device_id(glove, receiver_id, sizeof(receiver_id))) {
                    std::memcpy(receiver_id_cache, receiver_id, sizeof(receiver_id_cache));
                    std::fprintf(stderr, "receiver_device_id=%s\n", receiver_id_cache);
                    device_id_shown = true;
                }
            }
            if (!device_id_shown && (tick == 5 || tick == 30))
                hex_glove_request_device_id(glove);
            if (!route_requested && (device_id_shown || tick >= 20)) {
                hex_glove_send_get_route(glove);
                route_requested = true;
            }
            if (!hex_external_update(external)) { print_error(external); result = 7; break; }
            HexExternalFrame frame{};
            frame.struct_size = sizeof(frame);
            if (!hex_external_get_frame(external, &frame)) { print_error(external); result = 8; break; }
            HexGloveDeviceSnapshot device{};
            hex_glove_get_device_snapshot(glove, &device);
            const uint32_t online = (device.left_enabled ? 1u : 0u) | (device.right_enabled ? 2u : 0u);
            if ((!profile || request_r) && !r_started && online) {
                r_started = hex_mocap_start_r_capture(mocap, int(online), 1.f) != 0;
            }
            HexMocapRCaptureInfo r{};
            r.struct_size = sizeof(r);
            hex_mocap_get_r_capture_info(mocap, &r);
            const bool r_ready = (profile && !request_r) || r.state == 2;
            const uint32_t need_zero = frame.tracked_hand_mask & frame.fresh_hand_mask & ~calibrated_mask;
            if (r_ready && need_zero && hex_external_calibrate_angles(external, need_zero)) {
                calibrated_mask |= need_zero;
                std::fprintf(stderr, "Angle zero captured for mask %u\n", need_zero);
            }
            if (tick % 10 == 0) {
                std::fprintf(stderr, "receiver_id=%s\n", device_id_shown ? receiver_id_cache : "waiting");
                HexGloveReceiveDiagnostics diag{};
                diag.struct_size = sizeof(diag);
                hex_glove_get_receive_diagnostics(glove, &diag);
                std::fprintf(stderr, "status=%08x online=%u tracked=%u angles=%u bytes=%llu frames=%llu checksum=%llu\n",
                    hex_mocap_get_status(mocap), online, frame.tracked_hand_mask, frame.angle_valid_mask,
                    static_cast<unsigned long long>(diag.bytes), static_cast<unsigned long long>(diag.frames),
                    static_cast<unsigned long long>(diag.checksum_mismatches));
            }
            using namespace std::chrono;
            const auto utc_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            for (int h = 0; h < 2; ++h) if ((frame.angle_valid_mask & frame.fresh_hand_mask) & (1u << h)) {
                const auto& pose = frame.pose[h];
                const char* side = h ? "right" : "left";
                if (csv) {
                    std::fprintf(csv, "%lld,%lld,%llu,%u,%s", static_cast<long long>(pose.timestamp_ms),
                        static_cast<long long>(utc_ms), static_cast<unsigned long long>(frame.update_index), pose.frame_sequence, side);
                    for (float angle : frame.flexion_deg[h]) std::fprintf(csv, ",%.6f", angle);
                    std::fputc('\n', csv); std::fflush(csv);
                }
                if (jsonl) {
                    std::fprintf(jsonl, "{\"timestamp_ms\":%lld,\"utc_timestamp_ms\":%lld,\"update_index\":%llu,\"frame_sequence\":%u,\"hand\":\"%s\"",
                        static_cast<long long>(pose.timestamp_ms), static_cast<long long>(utc_ms),
                        static_cast<unsigned long long>(frame.update_index), pose.frame_sequence, side);
                    for (unsigned k = 0; k < HEX_EXTERNAL_ANGLE_COUNT; ++k)
                        std::fprintf(jsonl, ",\"%s\":%.6f", kNames[k], frame.flexion_deg[h][k]);
                    std::fputs("}\n", jsonl); std::fflush(jsonl);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
cleanup:
    if (csv) std::fclose(csv);
    if (jsonl && jsonl != stdout) std::fclose(jsonl);
    hex_external_destroy(external);
    hex_mocap_destroy(mocap);
    hex_glove_abort(glove);
    hex_glove_destroy(glove);
    return result;
}
