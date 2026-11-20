#pragma once
#include "hex_glove/mocap_model.h"

/* Lossless legacy trajectory storage. Capture retains its own bounded window. */
#define HEX_MOCAP_MAX_CONTACT_TRAJECTORY_SAMPLES 65536u
#define HEX_MOCAP_MAX_PROFILE_BYTES (160u * 1024u * 1024u)

enum HexMocapContactGateMode {
    HEX_MOCAP_CONTACT_CURRENT_UNITY = 0,
    HEX_MOCAP_CONTACT_NORMALIZED_EXPERIMENTAL = 1
};
enum HexMocapContactCalibFlags {
    HEX_MOCAP_CONTACT_HAS_RANGE = 1,
    HEX_MOCAP_CONTACT_HAS_MCP_EXTRA = 2,
    HEX_MOCAP_CONTACT_HAS_THUMB_EXTRA = 4
};
typedef struct HexMocapContactCalibration {
    uint32_t struct_size;
    uint32_t version;
    uint32_t flags;
    int32_t gate_mode;
    uint32_t curve_count;
    float curve_flex_deg[16];
    float curve_extra_deg[16];
    float mcp_min_deg, mcp_max_deg;
    float dip_min_deg, dip_max_deg;
    float flex_min_deg, flex_max_deg;
    float pip_bias_deg;
    float mcp_follow_ratio;
    HexMocapVec3 mcp_extra_deg;
    HexMocapVec3 thumb_extra_deg[3];
} HexMocapContactCalibration;

/** Direct representation of current Unity ContactCloseSample. All angles are
 * degrees; paired/thumbnail Euler offsets are CCD delta in local axes.
 */
typedef struct HexMocapContactFitSample {
    float mcp_deg, dip_deg, flex_sum_deg, tip_distance_m;
    float target_pip_deg, glove_pip_deg;
    float mcp_after_deg, pip_after_deg, dip_after_deg;
    HexMocapVec3 thumb_extra_deg[3];
    HexMocapVec3 finger_extra_deg[3];
} HexMocapContactFitSample;

typedef struct HexMocapContactInput {
    float mcp_deg, dip_deg;
    HexQuat thumb_root, thumb_tip, finger_root;
} HexMocapContactInput;
typedef struct HexMocapContactSample {
    HexMocapContactInput input;
    HexQuat closed_local[6];
    HexQuat driven_local[6];
    uint32_t has_driven;
    float residual_m;
} HexMocapContactSample;
typedef struct HexMocapContactMatch {
    int32_t first, second;
    float t, distance, position;
} HexMocapContactMatch;

typedef struct HexMocapThumbLinearJoint {
    uint32_t ready, uses_source;
    HexQuat reference, output_reference;
    float coefficient[3][4];
    float max_correction_deg;
} HexMocapThumbLinearJoint;
typedef struct HexMocapThumbLinearModel {
    uint32_t struct_size, version;
    HexMocapThumbLinearJoint joints[3];
} HexMocapThumbLinearModel;
typedef struct HexMocapThumbLinearState {
    uint32_t struct_size;
    uint32_t initialized[3];
    HexQuat stable_source[3], stable_driven[3];
} HexMocapThumbLinearState;
typedef struct HexMocapContactState {
    uint32_t struct_size;
    int32_t active_finger; /**< -1 or index..little; current personal owner in quality >=11. */
    float weight; /**< Personal confidence in quality >=11; legacy owner blend otherwise. */
    int32_t releasing;
    uint32_t trajectory_count[4];
    int32_t capturing_finger;
    uint32_t captured_count, rejected_count;
    float captured_flex_span_deg;
} HexMocapContactState;

enum HexMocapContactCapturePreset {
    HEX_MOCAP_CAPTURE_LEGACY_CANDIDATE = 0,
    HEX_MOCAP_CAPTURE_CURRENT_UNITY = 1
};
typedef struct HexMocapContactCaptureConfig {
    uint32_t struct_size, version, preset;
    uint32_t minimum_total_samples, minimum_close_samples;
    float minimum_close_ratio, minimum_flex_span_deg, close_distance_m;
    float preparation_seconds;
    uint32_t sample_interval_ms;
    float progress_per_second;
} HexMocapContactCaptureConfig;
enum HexMocapContactCapturePending {
    HEX_MOCAP_CAPTURE_NOT_ACTIVE = 1,
    HEX_MOCAP_CAPTURE_PREPARING = 2,
    HEX_MOCAP_CAPTURE_NEEDS_TOTAL = 4,
    HEX_MOCAP_CAPTURE_NEEDS_CLOSE = 8,
    HEX_MOCAP_CAPTURE_NEEDS_FLEX = 16,
    HEX_MOCAP_CAPTURE_NEEDS_RATIO = 32,
    HEX_MOCAP_CAPTURE_NEEDS_TRAJECTORY = 64,
    HEX_MOCAP_CAPTURE_NEEDS_DISTAL = 128,
    HEX_MOCAP_CAPTURE_NEEDS_VERIFICATION = 256,
    HEX_MOCAP_CAPTURE_NEEDS_NATURAL_POSE = 512,
    HEX_MOCAP_CAPTURE_NEEDS_RETURN = 1024,
    HEX_MOCAP_CAPTURE_TIMED_OUT = 2048
};
typedef struct HexMocapContactCaptureStatus {
    uint32_t struct_size, version;
    int32_t finger;
    uint32_t ready, pending_mask, total_samples, close_samples, stored_samples, rejected_samples;
    float flex_span_deg, flex_rise_deg, close_ratio, elapsed_seconds, progress;
} HexMocapContactCaptureStatus;

/* Diagnostic ABI: 1 strict verification; 2 simple closure; 3 limited motion;
 * 4 source Unity complete-pose replay (profile v12).
 * phase 0 capture, 1 release, 2 verify, 3 accepted, 4 conflict, 5 timed out.
 * Only independently verified data sets verified_mask. Distances in metres. */
typedef struct HexMocapContactQuality {
    uint32_t struct_size, version, verified_mask, phase;
    float mcp_span_deg, distal_span_deg, verification_ratio, verification_span_deg;
    float tip_distance_m, tolerance_m, required_span_deg;
} HexMocapContactQuality;
