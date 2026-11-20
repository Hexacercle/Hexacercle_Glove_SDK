#pragma once
#include "hex_glove/hex_mocap_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEX_EXTERNAL_API_VERSION 0x00010000u
#define HEX_EXTERNAL_ANGLE_COUNT 15u

/* Angle slots, in degrees, use one JointAngleHub definition for both hands. */
enum HexExternalAngleIndex {
    HEX_EXTERNAL_THUMB_CMC = 0, HEX_EXTERNAL_THUMB_MCP = 1, HEX_EXTERNAL_THUMB_IP = 2,
    HEX_EXTERNAL_INDEX_MCP = 3, HEX_EXTERNAL_INDEX_PIP = 4, HEX_EXTERNAL_INDEX_DIP = 5,
    HEX_EXTERNAL_MIDDLE_MCP = 6, HEX_EXTERNAL_MIDDLE_PIP = 7, HEX_EXTERNAL_MIDDLE_DIP = 8,
    HEX_EXTERNAL_RING_MCP = 9, HEX_EXTERNAL_RING_PIP = 10, HEX_EXTERNAL_RING_DIP = 11,
    HEX_EXTERNAL_LITTLE_MCP = 12, HEX_EXTERNAL_LITTLE_PIP = 13, HEX_EXTERNAL_LITTLE_DIP = 14
};

enum HexExternalErrorCode {
    HEX_EXTERNAL_OK = 0,
    HEX_EXTERNAL_INVALID_ARGUMENT = 1,
    HEX_EXTERNAL_BAD_STRUCT_SIZE = 2,
    HEX_EXTERNAL_PORT_OPEN_FAILED = 3,
    HEX_EXTERNAL_PROFILE_LOAD_FAILED = 4,
    HEX_EXTERNAL_UPDATE_FAILED = 5,
    HEX_EXTERNAL_NO_CAPTURED_FRAME = 6,
    HEX_EXTERNAL_NO_TRACKED_HAND = 7,
    HEX_EXTERNAL_ANGLE_CALIBRATION_FAILED = 8,
    HEX_EXTERNAL_NO_FRESH_FRAME = 9
};

typedef struct HexExternalFrame {
    uint32_t struct_size;
    uint32_t version;
    uint64_t update_index;       /* One successful capture call; not a device sequence. */
    int64_t host_timestamp_ms;   /* Monotonic host time of capture, not Unix UTC. */
    uint32_t mocap_status;
    uint32_t tracked_hand_mask;  /* 1 left, 2 right: at least one tracked joint. */
    uint32_t fresh_hand_mask;    /* 1 left, 2 right: new device frame in latest update. */
    uint32_t angle_valid_mask;   /* 1 left, 2 right: all 15 angle fields valid. */
    HexMocapDisplayPose pose[2]; /* Each hand retains its own SDK timestamp/sequence. */
    float flexion_deg[2][HEX_EXTERNAL_ANGLE_COUNT];
} HexExternalFrame;

typedef struct HexExternalErrorInfo {
    uint32_t struct_size;
    uint32_t code;               /* HexExternalErrorCode */
    char message_utf8[128];
} HexExternalErrorInfo;

typedef struct HexExternalSession* HexExternalHandle;

/* Non-owning: destroy this handle before the mocap and glove handles. Use all
 * operations on one thread. The mocap handle must use the supplied glove. */
HEX_GLOVE_API HexExternalHandle hex_external_create(HexGloveHandle glove, HexMocapHandle mocap);
HEX_GLOVE_API void hex_external_destroy(HexExternalHandle handle);
HEX_GLOVE_API uint32_t hex_external_get_api_version(void);

/* Optional wrappers with explicit errors for common setup failures. */
HEX_GLOVE_API int hex_external_start(HexExternalHandle handle, const char* port_utf8);
HEX_GLOVE_API int hex_external_profile_load(HexExternalHandle handle, const char* path_utf8);
/* Clear cached frame and Hub zero points after an externally managed reconnect. */
HEX_GLOVE_API int hex_external_reset(HexExternalHandle handle);

/* Recommended live path: one mocap update, then copy both final poses and Hub
 * angles into a cached frame. get_frame never advances the solver. A hand may
 * be absent/untracked; inspect masks before consuming its fields. */
HEX_GLOVE_API int hex_external_update(HexExternalHandle handle);
/* Use only when the application has already called hex_mocap_update itself. */
HEX_GLOVE_API int hex_external_capture(HexExternalHandle handle);
HEX_GLOVE_API int hex_external_get_frame(HexExternalHandle handle, HexExternalFrame* output);

/* After sensor R, hold selected hands in a straight neutral pose. Calibrates
 * both Hub and the SDK precise-angle baseline from the last captured poses. */
HEX_GLOVE_API int hex_external_calibrate_angles(HexExternalHandle handle, uint32_t hand_mask);

/* Last error belongs to this external handle and these wrapper calls only.
 * It does not diagnose arbitrary legacy SDK calls. */
HEX_GLOVE_API int hex_external_get_last_error(HexExternalHandle handle, HexExternalErrorInfo* output);
HEX_GLOVE_API const char* hex_external_error_string(uint32_t code);

#ifdef __cplusplus
}
#endif
