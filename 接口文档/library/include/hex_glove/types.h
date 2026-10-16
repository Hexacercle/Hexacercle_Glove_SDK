#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HexQuat {
    float x;
    float y;
    float z;
    float w;
} HexQuat;

typedef enum HexGloveImuLayout {
    HEX_GLOVE_LAYOUT_UNKNOWN = 0,
    HEX_GLOVE_LAYOUT_SEVEN = 7,
    HEX_GLOVE_LAYOUT_ELEVEN = 11
} HexGloveImuLayout;

/** Product line hint: 0=unknown, 7=M7, 11=M11. */
typedef enum HexGloveProductLine {
    HEX_GLOVE_PRODUCT_UNKNOWN = 0,
    HEX_GLOVE_PRODUCT_M7 = 7,
    HEX_GLOVE_PRODUCT_M11 = 11
} HexGloveProductLine;

enum { HEX_GLOVE_HW_CHECK_BYTES = 18 };

/**
 * One-shot device meta for Unity UI / environment check.
 * hw_check_* bytes: [0]=hw major,[1]=hw minor,[2]=sw major,[3]=sw minor,[4..5]=device code ASCII.
 * Hardware minor odd → M11, even → M7 (when version bytes present).
 */
typedef struct HexGloveDeviceSnapshot {
    int32_t left_enabled;
    int32_t right_enabled;
    int32_t left_new_node;
    int32_t right_new_node;
    int32_t layout_left;    /**< HexGloveImuLayout */
    int32_t layout_right;
    int32_t product_left;   /**< HexGloveProductLine */
    int32_t product_right;
    int32_t route;
    int32_t route_received;
    double batt_left;
    double batt_right;
    uint8_t hw_check_left[HEX_GLOVE_HW_CHECK_BYTES];
    uint8_t hw_check_right[HEX_GLOVE_HW_CHECK_BYTES];
    int32_t hw_check_left_valid;
    int32_t hw_check_right_valid;
} HexGloveDeviceSnapshot;

typedef struct HexSyncedGloveFrame {
    uint8_t frame_sequence;
    uint8_t left_sequence;
    uint8_t right_sequence;
    int32_t left_filled;
    int32_t right_filled;
    int64_t timestamp_ms;
    HexQuat left_fingers[6];
    HexQuat right_fingers[6];
} HexSyncedGloveFrame;

typedef struct HexSyncedGloveStats {
    int64_t total_synced_frames;
    int64_t left_filled_frames;
    int64_t right_filled_frames;
} HexSyncedGloveStats;

/** Root and NewNode copied under ONE decoder lock. Receive times are monotonic
 * host times of quaternion packets, not battery/metadata packet timestamps.
 * This is a latest-packet snapshot, not a hardware-synchronized pair.
 */
typedef struct HexGloveRawHandFrame {
    uint32_t struct_size;
    int32_t root_live;
    int32_t tip_live;
    int32_t layout;
    int64_t root_timestamp_ms;
    int64_t tip_timestamp_ms;
    uint32_t root_sequence;
    uint32_t tip_sequence;
    HexQuat root[6];
    HexQuat tip[6];
} HexGloveRawHandFrame;

/* Cumulative since session creation. Node order: left root, left tip,
 * right root, right tip. Checksum mismatches are observed, not newly rejected. */
typedef struct HexGloveReceiveDiagnostics {
    uint32_t struct_size, version;
    uint64_t bytes, frames, checksum_mismatches, framing_errors;
    uint64_t rotations[4];
} HexGloveReceiveDiagnostics;

#ifdef __cplusplus
}
#endif
