#pragma once
#include "hex_glove/hex_angles_api.h"
#include <chrono>
#include <cstdio>
#include <cstdint>

// JointAngleHub 按同一套 21 节点顺序计算双手角度；每只手保留独立零位。
// 这里的几何角零位独立于传感器 R，须在 R 完成后、手指伸直中立时采集。
class AngleOutput {
public:
    // 对外输出的 15 个关节槽位。finger[5][4] 没有单独的拇指 CMC，故这里用 Hub。
    static constexpr int count = 15;
    inline static constexpr int slots[count] = {1,2,3,5,6,7,9,10,11,13,14,15,17,18,19};
    inline static constexpr const char* names[count] = {
        "thumb_cmc","thumb_mcp","thumb_ip",
        "index_mcp","index_pip","index_dip",
        "middle_mcp","middle_pip","middle_dip",
        "ring_mcp","ring_pip","ring_dip",
        "little_mcp","little_pip","little_dip"};

    HexAngleHubConfig config[2]{};
    HexAngleHubState state[2]{};
    bool calibrated[2]{};
    int64_t last_timestamp[2]{};
    uint32_t last_sequence[2]{};
    std::FILE* csv = nullptr;
    std::FILE* jsonl = nullptr;

    AngleOutput() {
        // SDK 要求先填写 struct_size，再初始化配置和运行状态。
        for (int h = 0; h < 2; ++h) {
            config[h].struct_size = sizeof(config[h]);
            state[h].struct_size = sizeof(state[h]);
            hex_angle_hub_config_init(&config[h]);
            hex_angle_hub_state_init(&state[h]);
        }
    }
    bool open(const char* csv_path, const char* jsonl_path) {
        // 可同时写 CSV 和 JSON Lines；jsonl_path="-" 表示实时写标准输出。
        if (csv_path) {
            csv = std::fopen(csv_path, "w");
            if (!csv) return false;
            std::fputs("timestamp_ms,utc_timestamp_ms,frame_sequence,hand", csv);
            for (const char* name : names) std::fprintf(csv, ",%s", name);
            std::fputc('\n', csv);
        }
        if (jsonl_path) {
            jsonl = jsonl_path[0] == '-' && jsonl_path[1] == '\0'
                ? stdout : std::fopen(jsonl_path, "w");
            if (!jsonl) return false;
        }
        return true;
    }
    ~AngleOutput() {
        // 只关闭本类打开的文件，不关闭调用方可能正在使用的 stdout。
        if (csv) std::fclose(csv);
        if (jsonl && jsonl != stdout) std::fclose(jsonl);
    }
    void reset(int hand) {
        // 重新 R 或断开重连后清掉该手角度零位和上一帧标记。
        calibrated[hand] = false;
        last_timestamp[hand] = 0;
        last_sequence[hand] = 0;
        state[hand].struct_size = sizeof(state[hand]);
        hex_angle_hub_state_init(&state[hand]);
    }
    bool valid(const HexMocapDisplayPose& pose) const {
        // 所有将要输出的关节必须有真实跟踪位，不能把静态模型姿态当数据。
        for (int slot : slots) if (!(pose.tracked_mask & (1u << slot))) return false;
        return true;
    }
    bool calibrate(int hand, const HexMocapDisplayPose& pose) {
        // 用当前最终骨骼姿态建立该手 Hub 几何角零位。
        if (!valid(pose)) return false;
        calibrated[hand] = hex_angle_hub_calibrate(&config[hand], &state[hand], &pose, nullptr) != 0;
        last_timestamp[hand] = pose.timestamp_ms;
        last_sequence[hand] = pose.frame_sequence;
        return calibrated[hand];
    }
    bool update(int hand, const HexMocapDisplayPose& pose) {
        // 只处理时间戳和帧号都前进的新帧；dt 以秒传给 Hub 滤波器。
        if (!calibrated[hand] || !valid(pose) || pose.timestamp_ms <= last_timestamp[hand] ||
            pose.frame_sequence == last_sequence[hand]) return false;
        const float dt = static_cast<float>(pose.timestamp_ms - last_timestamp[hand]) / 1000.f;
        if (dt > 10.f || !hex_angle_hub_update(&config[hand], &state[hand], &pose, dt, nullptr, nullptr)) return false;
        last_timestamp[hand] = pose.timestamp_ms;
        last_sequence[hand] = pose.frame_sequence;
        for (int slot : slots) if (!(state[hand].valid_mask & (1u << slot))) return false;
        write(hand, pose);
        return true;
    }
    void write(int hand, const HexMocapDisplayPose& pose) {
        // SDK timestamp_ms 为单调时间；UTC 毫秒由宿主时钟单独提供。
        using namespace std::chrono;
        const auto utc_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        if (csv) {
            std::fprintf(csv, "%lld,%lld,%u,%s", static_cast<long long>(pose.timestamp_ms),
                static_cast<long long>(utc_ms), pose.frame_sequence, hand ? "right" : "left");
            for (int slot : slots) std::fprintf(csv, ",%.6f", state[hand].flexion[slot]);
            std::fputc('\n', csv);
            std::fflush(csv);
        }
        if (jsonl) {
            std::fprintf(jsonl, "{\"timestamp_ms\":%lld,\"utc_timestamp_ms\":%lld,\"frame_sequence\":%u,\"hand\":\"%s\"",
                static_cast<long long>(pose.timestamp_ms), static_cast<long long>(utc_ms),
                pose.frame_sequence, hand ? "right" : "left");
            for (int k = 0; k < count; ++k)
                std::fprintf(jsonl, ",\"%s\":%.6f", names[k], state[hand].flexion[slots[k]]);
            std::fputs("}\n", jsonl);
            std::fflush(jsonl);
        }
    }
};
