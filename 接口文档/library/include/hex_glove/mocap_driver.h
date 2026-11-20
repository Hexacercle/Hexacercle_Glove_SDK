#pragma once
#include <stdint.h>
#include "hex_glove/types.h"

/** Stage B driver configuration. Initialize with hex_mocap_driver_config_init.
 * preset=0 preserves stage A tuning; preset=1 is the current Unity candidate.
 * Flags/angles are runtime configuration, not a claim of visual equivalence.
 */
typedef struct HexMocapDriverConfig {
    uint32_t struct_size;
    uint32_t version;
    int32_t current_unity;
    int32_t unlock_pinky_lateral;
    int32_t use_unity_kalman; /**< 1=adaptive palm-relative filter for live/V2 input (V1 retains DLL scalar Kalman), 0=Slerp alpha */
    float sensor_slerp_alpha; /**< 1 bypasses SDK approximation; NOT Unity Kalman. */
    float thumb_root_post[3]; /**< right, left mirrors y/z */
    float thumb_mid_post[3];
    float thumb_tip_post[3];
    float thumb_swing_gain;
    float thumb_tip_extra_deg;
    float thumb_palmward_max_deg;
    float thumb_y_follow_full_deg;
    float mcp_extra_deg[4]; /**< explicit index/middle/ring/little offsets; preset 1 defaults to zero, independent of size */
    float mcp_coupling_gain;
} HexMocapDriverConfig;

enum HexMocapHandDriverFlags {
    HEX_MOCAP_HAND_PRESENT = 1,
    HEX_MOCAP_HAND_INPUT_VALID = 2,
    HEX_MOCAP_HAND_HAS_R = 4,
    HEX_MOCAP_HAND_R_SAMPLING = 8,
    HEX_MOCAP_HAND_M7_REAL_INPUT = 16,
    HEX_MOCAP_HAND_LAYOUT_CHANGED = 32
};
typedef struct HexMocapHandDriverState {
    uint32_t struct_size;
    uint32_t flags;
    int32_t layout;
    uint32_t frame_sequence;
    int64_t timestamp_ms;
    uint32_t generation; /**< increments when input/lifecycle resets */
    uint32_t reserved;
} HexMocapHandDriverState;

enum HexMocapRCaptureState {
    HEX_MOCAP_R_IDLE = 0, HEX_MOCAP_R_SAMPLING = 1,
    HEX_MOCAP_R_COMPLETE = 2, HEX_MOCAP_R_FAILED = 3, HEX_MOCAP_R_CANCELLED = 4
};
typedef struct HexMocapRCaptureInfo {
    uint32_t struct_size;
    int32_t state;
    int32_t hand_mask;
    float progress;
    uint32_t fresh_hand_mask;
} HexMocapRCaptureInfo;

/** Separate, versioned diagnostics; HexMocapRCaptureInfo remains 20 bytes. */
enum HexMocapRDiagnosticStage {
    HEX_MOCAP_R_STAGE_IDLE=0, HEX_MOCAP_R_STAGE_ACQUIRE=1, HEX_MOCAP_R_STAGE_SAMPLE=2,
    HEX_MOCAP_R_STAGE_MODEL=3, HEX_MOCAP_R_STAGE_NEUTRAL=4, HEX_MOCAP_R_STAGE_PIPELINE=5,
    HEX_MOCAP_R_STAGE_COMPLETE=6, HEX_MOCAP_R_STAGE_CANCELLED=7
};
enum HexMocapRDiagnosticReason {
    HEX_MOCAP_R_REASON_NONE=0, HEX_MOCAP_R_REASON_INVALID_REQUEST=1,
    HEX_MOCAP_R_REASON_NO_INPUT=2, HEX_MOCAP_R_REASON_HAND_UNAVAILABLE=3,
    HEX_MOCAP_R_REASON_TIMEOUT=4, HEX_MOCAP_R_REASON_EMPTY=5,
    HEX_MOCAP_R_REASON_INSUFFICIENT=6, HEX_MOCAP_R_REASON_UNSTABLE=7,
    HEX_MOCAP_R_REASON_LAYOUT_CHANGED=8, HEX_MOCAP_R_REASON_STALE=9,
    HEX_MOCAP_R_REASON_MODEL_INCOMPATIBLE=10, HEX_MOCAP_R_REASON_MODEL_INVALID=11,
    HEX_MOCAP_R_REASON_MODEL_PREPARATION=12, HEX_MOCAP_R_REASON_MODEL_REFERENCE=13,
    HEX_MOCAP_R_REASON_ANGLE_NEUTRAL=14, HEX_MOCAP_R_REASON_PIPELINE=15,
    HEX_MOCAP_R_REASON_ALLOCATION=16, HEX_MOCAP_R_REASON_BUSY=17, HEX_MOCAP_R_REASON_CANCELLED=18
};
typedef struct HexMocapRCaptureDiagnostic {
    uint32_t struct_size,version,attempt;
    int32_t stage,reason,hand_mask,failed_hand; /**< failed_hand: -1 or 0(left)/1(right). */
    int32_t sampler_reason,model_reason; /**< Internal reason detail, diagnostic only. */
    uint32_t fresh_hand_mask,samples,inliers;
    int64_t span_ms,age_ms;
    float max_deviation_deg,max_rms_deg,max_trend_deg_s;
    uint32_t quality_version,pipeline_failed_mask,reserved;
} HexMocapRCaptureDiagnostic;

/** Set struct_size, previous=baseline, covariance={10,10,10,10}, gain={0,0,0,0}.
 * filter_quaternion updates all fields atomically and returns output in previous.
 * Caller must maintain quaternion hemisphere continuity before filtering.
 */
typedef struct HexMocapKalmanState {
    uint32_t struct_size;
    HexQuat previous;
    HexQuat covariance;
    HexQuat gain;
} HexMocapKalmanState;
