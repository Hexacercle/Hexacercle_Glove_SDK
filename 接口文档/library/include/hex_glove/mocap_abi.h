#pragma once
#include <stdint.h>

#define HEX_MOCAP_API_VERSION 0x00010400u
#define HEX_MOCAP_INPUT_VERSION 1u
#define HEX_MOCAP_INPUT_VERSION_LATEST 2u

/** Capabilities describe callable primitives, not full Unity parity. */
enum HexMocapCapability {
    HEX_MOCAP_CAP_OFFLINE_FRAME = 1 << 0,
    HEX_MOCAP_CAP_WORLD_ROTATIONS = 1 << 1,
    HEX_MOCAP_CAP_DISTAL_SCALAR = 1 << 2,
    HEX_MOCAP_CAP_M11_DISTAL_PARTIAL = 1 << 3,
    HEX_MOCAP_CAP_LEGACY_CALIBRATION = 1 << 4,
    HEX_MOCAP_CAP_M7_RAW_INPUT = 1 << 5,
    HEX_MOCAP_CAP_DRIVER_CONFIG = 1 << 6,
    HEX_MOCAP_CAP_TIMED_R = 1 << 7,
    HEX_MOCAP_CAP_FK_GEOMETRY = 1 << 8,
    HEX_MOCAP_CAP_PROFILE = 1 << 9,
    HEX_MOCAP_CAP_PRESSURE = 1 << 10,
    HEX_MOCAP_CAP_RECORDING = 1 << 11,
    HEX_MOCAP_CAP_MODEL_PIPELINE = 1 << 12,
    HEX_MOCAP_CAP_PRECISE_ANGLES = 1 << 13,
    HEX_MOCAP_CAP_LEGACY_ANIMATION = 1 << 14,
    HEX_MOCAP_CAP_SUBSCRIPTIONS = 1 << 15,
    HEX_MOCAP_CAP_DEVICE_WORKFLOWS = 1 << 16,
    HEX_MOCAP_CAP_RETARGET = 1 << 17
};

/** uint32-only layout, no pointers. Caller sets struct_size before querying.
 * Sizes/offsets describe the loaded library and can be checked by C#/Qt.
 */
typedef struct HexMocapAbiInfo {
    uint32_t struct_size;
    uint32_t api_version;
    uint32_t input_version;
    uint32_t pointer_size;
    uint32_t quat_size;
    uint32_t input_hand_size;
    uint32_t input_frame_size;
    uint32_t skeleton_size;
    uint32_t joint_angles_size;
    uint32_t calib_size;
    uint32_t input_timestamp_offset;
    uint32_t input_left_offset;
    uint32_t input_right_offset;
    uint32_t hand_root_offset;
    uint32_t hand_tip_offset;
    uint32_t skeleton_left_offset;
    uint32_t skeleton_right_offset;
    uint32_t skeleton_status_offset;
    uint32_t angles_left_offset;
    uint32_t angles_right_offset;
    uint32_t angles_status_offset;
    uint32_t real_joint_count;
    uint32_t display_joint_count;
} HexMocapAbiInfo;
