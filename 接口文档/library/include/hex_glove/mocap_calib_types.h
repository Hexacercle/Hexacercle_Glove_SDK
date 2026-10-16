#pragma once

/**
 * M2–M6: per-hand calibration *results* stored in C++ (authority).
 * Unity UI still runs sampling / tip-pinch detect; upload via set_* after Done.
 * M3: solve consumes tip bias / finger_delta / ROM map; ring → display bone scales.
 * M6: tip-contact polys + per-frame gate; Anthropometric → UploadRingScales.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum HexMocapCalibFlags {
    HEX_MOCAP_CALIB_HAS_R = 1 << 0,
    HEX_MOCAP_CALIB_HAS_ROM = 1 << 1,
    HEX_MOCAP_CALIB_HAS_RING = 1 << 2,
    HEX_MOCAP_CALIB_HAS_TIP = 1 << 3,
    /* Independent Unity wear ROM maps; HAS_ROM means both are ready. */
    HEX_MOCAP_CALIB_HAS_MCP_ROM = 1 << 4,
    HEX_MOCAP_CALIB_HAS_DIP_ROM = 1 << 5
};

/** Extra pending_mask bits when an 825 seven-step calibration has failed. */
enum HexMocapSevenStepFailure {
    HEX_MOCAP_SEVEN_NO_POSE = 1u << 16,
    HEX_MOCAP_SEVEN_NOT_CLOSED = 1u << 17,
    HEX_MOCAP_SEVEN_CONFLICTING_POSE = 1u << 18,
    HEX_MOCAP_SEVEN_FAILED_LEFT = 1u << 24,
    HEX_MOCAP_SEVEN_FAILED_RIGHT = 1u << 25
};

/** HandController seven-step automatic gesture calibration.
 * state: 0 idle, 1 running, 2 complete, 3 cancelled, 4 failed.
 * step: 1 flat/R, 2 open ROM, 3 fist ROM, 4..7 index..little contact.
 * pending_mask uses HexMocapContactCapturePending and HexMocapSevenStepFailure.
 */
typedef struct HexMocapSevenStepState {
    uint32_t struct_size, version, state, step, hand_mask, completed_steps;
    float progress;
    uint32_t preparing, pending_mask, total_samples, close_samples;
    float flex_span_deg;
} HexMocapSevenStepState;

/** TipContactStyle — matches GlovePipFromMcpDipEstimator.TipContactStyle. */
enum HexMocapTipContactStyle {
    HEX_MOCAP_TIP_ELONGATED = 0,
    HEX_MOCAP_TIP_TIGHT_CIRCLE = 1,
    HEX_MOCAP_TIP_SMOOTH_ARC = 2,
    HEX_MOCAP_TIP_DIP_LED = 3,
    HEX_MOCAP_TIP_MCP_LED = 4,
    HEX_MOCAP_TIP_BALANCED = 5
};

/**
 * Finger order: 0=index, 1=middle, 2=ring, 3=little (same as GloveMcpFlexRomSession).
 * All angles in degrees.
 */
typedef struct HexMocapHandCalibResults {
    uint32_t flags;

    /* ROM (MCP + DIP flex rest/peak), from GloveMcpFlexRomSession */
    float mcp_flex_rest_deg[4];
    float mcp_flex_peak_deg[4];
    float mcp_flex_dir[4];
    float dip_flex_rest_deg[4];
    float dip_flex_peak_deg[4];
    float dip_flex_dir[4];

    /* Ring / length scales (1 = unchanged) */
    float finger_length_scale[4];
    float palm_scale;

    /* Tip-contact habit */
    int32_t tip_style[4];
    float tip_pip_bias_deg[4];

    /* Global second-joint follow (FingerDelta) + per-finger offset */
    float finger_delta;
    float pip_blend_offset[4];
} HexMocapHandCalibResults;

#ifdef __cplusplus
}
#endif
