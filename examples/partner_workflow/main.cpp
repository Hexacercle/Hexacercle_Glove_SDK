#include "hex_glove/hex_partner_api.h"
#include "hex_glove/hex_hardware_info_api.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <thread>

static int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "Usage: partner_workflow COM3|auto PROFILE_DIR [--r] [--hand left|right|both] [--seconds N]\n");
        return 1;
    }
    int seconds = 30;
    bool request_r = false;
    uint32_t r_hand_mask = 3;
    for (int i = 3; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--r")) request_r = true;
        else if (!std::strcmp(argv[i], "--hand") && i + 1 < argc) {
            const char* side = argv[++i];
            r_hand_mask = !std::strcmp(side, "left") ? 1u : !std::strcmp(side, "right") ? 2u :
                          !std::strcmp(side, "both") ? 3u : 0u;
            if (!r_hand_mask) return 1;
        }
        else if (!std::strcmp(argv[i], "--seconds") && i + 1 < argc) seconds = std::atoi(argv[++i]);
        else return 1;
    }
    HexPartnerConfig config{};
    config.struct_size = sizeof(config);
    if (!hex_partner_config_init(&config)) return 2;
    if (std::strcmp(argv[1], "auto")) {
        config.auto_detect = 0;
        std::snprintf(config.preferred_port_utf8, sizeof(config.preferred_port_utf8), "%s", argv[1]);
    }
    std::snprintf(config.profile_dir_utf8, sizeof(config.profile_dir_utf8), "%s", argv[2]);

    HexGloveHandle glove = hex_glove_create();
    HexMocapHandle mocap = glove ? hex_mocap_create(glove) : nullptr;
    HexExternalHandle snapshot = mocap ? hex_external_create(glove, mocap) : nullptr;
    HexPartnerHandle partner = snapshot ? hex_partner_create(glove, mocap, snapshot, &config) : nullptr;
    if (!partner) {
        std::fprintf(stderr, "Cannot initialize SDK workflow\n");
        if (snapshot) hex_external_destroy(snapshot);
        if (mocap) hex_mocap_destroy(mocap);
        if (glove) hex_glove_destroy(glove);
        return 3;
    }
    int result = 0;
    if (!hex_partner_connect(partner, now_ms())) result = 4;
    uint32_t event_sequence = 0;
    bool r_started = false;
    for (int tick = 0; !result && tick < seconds * 20; ++tick) {
        // tick 管理搜索、自动重连、姿态更新及 R 进度；由同一线程调用。
        if (!hex_partner_tick(partner, now_ms())) {
            std::fprintf(stderr, "tick failed\n"); result = 5; break;
        }
        HexPartnerStatus status{};
        status.struct_size = sizeof(status);
        if (!hex_partner_get_status(partner, &status)) { result = 6; break; }
        HexPartnerEvent event{};
        event.struct_size = sizeof(event);
        while (hex_partner_get_event(partner, event_sequence, &event) == 1) {
            event_sequence = event.sequence;
            std::fprintf(stderr, "event=%u error=%u (%s) port=%s\n", event.kind, event.error,
                         event.message_utf8, status.port_utf8);
        }
        char receiver_id[7]{};
        if (tick % 20 == 0)
            std::fprintf(stderr, "phase=%u receiver_id=%s online=%u fresh=%u streaming=%u profile=%u left_code=%s right_code=%s\n",
                status.phase, hex_glove_get_device_id(glove, receiver_id, sizeof(receiver_id)) ? receiver_id : "waiting",
                status.online_hand_mask, status.fresh_hand_mask,
                status.streaming_hand_mask, status.profile_loaded,
                status.device_code[0], status.device_code[1]);

        // 双手同时 R 需要两只手均在持续输出；单手只传对应的一位。
        if (request_r && !r_started && (status.streaming_hand_mask & r_hand_mask) == r_hand_mask) {
            r_started = hex_partner_calibration_begin(partner, r_hand_mask,
                        HEX_PARTNER_CALIBRATION_R, 1.f) != 0;
        }
        HexPartnerCalibrationStatus calib{};
        calib.struct_size = sizeof(calib);
        hex_partner_get_calibration(partner, &calib);
        if (tick % 20 == 0 && calib.result == HEX_PARTNER_CALIBRATION_RUNNING)
            std::fprintf(stderr, "R state=%u progress=%.0f%%\n", calib.r_state, calib.r_progress * 100.f);

        // 同一 tick 已缓存骨骼与角度；fresh 位阻止把旧帧当成新数据。
        HexExternalFrame frame{};
        frame.struct_size = sizeof(frame);
        if (status.streaming_hand_mask && hex_external_get_frame(snapshot, &frame)) {
            for (int hand = 0; hand < 2; ++hand) if (frame.fresh_hand_mask & (1u << hand)) {
                const auto& pose = frame.pose[hand];
                if (tick % 20 == 0)
                    std::printf("hand=%d seq=%u timestamp_ms=%lld joints=%u palm=(%.3f,%.3f,%.3f)\n",
                        hand, pose.frame_sequence, static_cast<long long>(pose.timestamp_ms),
                        pose.tracked_mask, pose.world_position[0].x,
                        pose.world_position[0].y, pose.world_position[0].z);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    hex_partner_disconnect(partner);
    hex_partner_destroy(partner);
    hex_external_destroy(snapshot);
    hex_mocap_destroy(mocap);
    hex_glove_destroy(glove);
    return result;
}
