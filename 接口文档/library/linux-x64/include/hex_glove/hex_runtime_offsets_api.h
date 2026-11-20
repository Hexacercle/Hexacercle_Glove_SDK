#pragma once
#include "hex_glove/hex_mocap_unity_extensions.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Canonical node arrays. Current HandController scene's live crossfade branch. */
typedef struct HexRuntimeOffsetsConfig {
    uint32_t struct_size, version, enabled;
    uint32_t thumb_only, use_per_finger_thumb, legacy_euler_smoothing;
    uint32_t preserve_mask, persisted_mask, tip_pip_mask, pair_mask[4];
    uint32_t pair_fallback_mask[4]; /* ContactClose extras: no bilateral/baseline transform. */
    HexMocapVec3 persisted[21], tip_pip[21], thumb_pair[4][4];
    uint32_t coeff_valid[4]; float coeff_a[4], coeff_b[4];
    float thumb_strength, finger_strength, natural_strength, snap_max, snap_gap_blend;
    float thumb_near, thumb_far, finger_near, finger_far;
    float thumb_smooth, thumb_speed, finger_smooth, finger_speed, finger_weight_smooth;
    float bend_start, bend_full, switch_hysteresis, ambiguous_enter, ambiguous_exit, switch_advantage;
    float enter_seconds, crossfade_seconds, crossfade_speed, crossfade_max_seconds;
    float engagement_far, engagement_hysteresis, engagement_smooth, sweep_attenuation;
    float tip_near, tip_far, tip_smooth;
    uint32_t baseline_valid; HexQuat stored_baseline;
} HexRuntimeOffsetsConfig;
typedef struct HexRuntimeOffsetsState {
    uint32_t struct_size, version, initialized_mask;
    int32_t locked, from_finger, to_finger;
    uint32_t ambiguous, engaged;
    float progress, engagement, engagement_velocity;
    float finger_weight[4], finger_velocity[4];
    HexMocapVec3 applied[21], velocity[21];
} HexRuntimeOffsetsState;
typedef struct HexRuntimeOffsetsFrame {
    uint32_t struct_size;
    float dt_seconds, flex_deg[4];
    int32_t exclusive_finger, capturing_finger; /* -1 or index..little */
    uint32_t has_trajectory, formula_mask, fist_suppressed, current_baseline_valid;
    HexQuat current_baseline;
    HexMocapVec3 current_offset[21];
} HexRuntimeOffsetsFrame;
typedef struct HexRuntimeOffsetsOutput {
    uint32_t struct_size, override_mask;
    HexMocapVec3 offset[21];
    int32_t dominant_finger;
    float thumb_weight;
} HexRuntimeOffsetsOutput;
HEX_GLOVE_API int hex_runtime_offsets_config_init(HexRuntimeOffsetsConfig* config);
HEX_GLOVE_API int hex_runtime_offsets_validate_config(const HexRuntimeOffsetsConfig* config);
HEX_GLOVE_API int hex_runtime_offsets_state_init(HexRuntimeOffsetsState* state);
/* Applies current Gesture profile dictionary/settings and bilateral mirroring.
 * Both entries must be initialized. Parsing/validation is transactional. */
HEX_GLOVE_API int hex_runtime_offsets_import_json(HexRuntimeOffsetsConfig config[2],const char* utf8,uint32_t bytes);
HEX_GLOVE_API int hex_runtime_offsets_process(const HexRuntimeOffsetsConfig* config,HexRuntimeOffsetsState* state,const HexMocapDisplayPose* pose,const HexRuntimeOffsetsFrame* frame,HexRuntimeOffsetsOutput* output);
#ifdef __cplusplus
}
#endif
