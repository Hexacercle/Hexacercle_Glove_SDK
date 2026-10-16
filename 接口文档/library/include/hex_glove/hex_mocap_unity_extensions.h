#pragma once
#include "hex_glove/hex_glove_api.h"
#include "hex_glove/mocap_contact.h"
#include "hex_glove/mocap_calib_types.h"
#include "hex_glove/hex_fourstep_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* All functions return 1 on success, 0 on rejected input unless documented.
 * Caller owns all buffers. On failure outputs and persistent state are unchanged. */
typedef struct HexMocapJointAffine { uint32_t valid; float scale, bias; } HexMocapJointAffine;
typedef struct HexMocapJointAffineSet { uint32_t struct_size; HexMocapJointAffine joint[3]; uint32_t used_samples; } HexMocapJointAffineSet;
HEX_GLOVE_API int hex_mocap_fit_joint_affine(int32_t finger, const HexMocapContactFitSample* samples, uint32_t count, HexMocapJointAffineSet* out);
/* Pure historic map; current Unity MCP/DIP runtime extras intentionally return zero. */
HEX_GLOVE_API int hex_mocap_evaluate_joint_affine(const HexMocapJointAffineSet* maps, const float before[3], float after[3]);

typedef struct HexMocapUnityScaleConfig {
    uint32_t struct_size, version;
    int32_t thumb_mode; /* 0 RawStep0, 1 CapToLongFingerMean, 2 BlendHighOutliers */
    int32_t palm_mode; /* 0 five-finger mean, 1 middle only, 2 skip */
    float thumb_max_ratio, thumb_blend, clamp_min, clamp_max;
    uint32_t thumb_gradient_enabled, apply_lengths, apply_step3_tweak;
    float thumb_gradient[4], step3_tweak[4];
    uint32_t preserve_scale_mask; /* canonical display slots; preserve current node scale */
} HexMocapUnityScaleConfig;
HEX_GLOVE_API int hex_mocap_unity_scale_config_init(HexMocapUnityScaleConfig* config);
/* Unity ScaleApplier writes uniform PER-NODE local scales (hierarchical).
 * current is immutable input; output can be passed to set_skeleton_definition.
 * measured/model order thumb,index,middle,ring,little; invalid lengths use ratio1. */
HEX_GLOVE_API int hex_mocap_apply_unity_scales(const HexMocapUnityScaleConfig* config, const float measured[5], const float model[5], const HexMocapSkeletonDefinition* current, HexMocapSkeletonDefinition* out, float effective_ratios[5]);

typedef struct HexMocapCcdConfig {
    uint32_t struct_size, version;
    uint32_t segment_assist, finger_assist;
    float fade_seconds, bend_start_deg, bend_full_deg;
    uint32_t euler_additive_mask; /* else deltaQuaternion * baseQuaternion */
    uint32_t fist_suppression, fist_min_fingers;
    float fist_zone_m, fist_ambiguity_m, fist_exit_extra_m;
} HexMocapCcdConfig;
typedef struct HexMocapCcdFrame {
    uint32_t struct_size;
    float dt_seconds, mcp_deg[4], dip_deg[4];
    uint32_t calibrated_finger_mask;
    int32_t replay_active; /* any trajectory runtime exists: skip legacy finger assist */
    int32_t replay_finger; /* -1 or index..little 0..3 */
    float replay_weight;
    HexMocapVec3 calibrated_mcp_extra[4];
    HexMocapVec3 base_offset[21];
} HexMocapCcdFrame;
typedef struct HexMocapCcdState {
    uint32_t struct_size;
    int32_t active_finger, segment_finger;
    float visual_weight, runtime_weight, segment_weight, glove_gap_m;
    HexMocapVec3 applied_offset[21];
} HexMocapCcdState;
typedef struct HexMocapCcdSession* HexMocapCcdHandle;
HEX_GLOVE_API int hex_mocap_ccd_config_init(HexMocapCcdConfig* config);
HEX_GLOVE_API HexMocapCcdHandle hex_mocap_ccd_create(const HexMocapCcdConfig* config);
HEX_GLOVE_API HexMocapCcdHandle hex_mocap_ccd_clone(HexMocapCcdHandle handle);
HEX_GLOVE_API int hex_mocap_ccd_get_config(HexMocapCcdHandle handle,HexMocapCcdConfig* out);
HEX_GLOVE_API int hex_mocap_ccd_set_config(HexMocapCcdHandle handle,const HexMocapCcdConfig* config);
HEX_GLOVE_API void hex_mocap_ccd_destroy(HexMocapCcdHandle handle);
HEX_GLOVE_API int hex_mocap_ccd_reset(HexMocapCcdHandle handle);
/* base_pose MUST exclude this handle's own applied offsets. May include replay.
 * SDK rebuilds full FK and never modifies the caller's baseline pose. */
HEX_GLOVE_API int hex_mocap_ccd_process(HexMocapCcdHandle handle, const HexMocapDisplayPose* base_pose, const HexMocapCcdFrame* frame, HexMocapDisplayPose* out_pose, HexMocapCcdState* out_state);
/* Exact low-level Unity CCD primitive, canonical chain 0=thumb..4=little. */
HEX_GLOVE_API int hex_mocap_ccd_solve(const HexMocapDisplayPose* base_pose, int32_t chain, HexMocapVec3 target, float limit_deg, uint32_t iterations, int32_t first_joint, int32_t end_joint, uint32_t anatomical, float converge_m, HexMocapDisplayPose* out_pose);
/* Current ForceLiveContactClosePoseNoRestore: mode 0=capture (10 rounds),
 * 1=replay residual (2), 2=live filtered (5). finger=index..little 0..3.
 * previous_filtered is optional and only read in mode2. filtered_pose retains
 * the pre-blend filtered result for the next frame; out_pose applies close_blend.
 * Capture commits require checking the resulting measured tip residual. */
HEX_GLOVE_API int hex_mocap_contact_close_solve(const HexMocapDisplayPose* base_pose, int32_t finger, uint32_t mode, uint32_t skip_thumb, float close_blend, float dt_seconds, const HexMocapDisplayPose* previous_filtered, HexMocapDisplayPose* filtered_pose, HexMocapDisplayPose* out_pose);

typedef struct HexMocapPivotSample { HexMocapVec3 pivot, thumb_tip, opposing_tip; HexQuat parent_world; float weight; } HexMocapPivotSample;
/* Weighted same-hemisphere local FromTo corrections, avg * baseline offset. */
HEX_GLOVE_API int hex_mocap_solve_thumb_pivot(const HexMocapPivotSample* samples, uint32_t count, HexQuat baseline_offset, HexQuat* out_offset);

typedef struct HexMocapUnityJson* HexMocapUnityJsonHandle;
typedef struct HexMocapUnityJsonInfo { uint32_t struct_size, schema_kind, schema_version, hand_mask; char user_id[128]; uint32_t trajectory_count[2][4]; uint32_t node_offset_count; } HexMocapUnityJsonInfo;
typedef struct HexMocapUnityWearRom {
    uint32_t struct_size, mcp_valid, dip_valid, thumb_valid, yaw_valid, habit_ready;
    float mcp_rest[4],mcp_peak[4],mcp_dir[4],dip_rest[4],dip_peak[4],dip_dir[4];
    float thumb_root_full,thumb_tip_full,yaw_abduct,yaw_adduct,bend_open,bend_closed;
    int32_t preferred_style[4], glove_size_index, is_seven_sensor;
} HexMocapUnityWearRom;
typedef struct HexMocapUnityNodeOffset { char node_name[128], pair_key[64]; uint32_t kind; HexMocapVec3 euler; } HexMocapUnityNodeOffset;
typedef struct HexMocapUnityGestureSettings {
    uint32_t struct_size;
    float mcp_min,mcp_max,dip_min,dip_max,habit_write_blend,habit_prefer_bias;
    float runtime_thumb_strength,runtime_finger_strength,natural_feel_strength,snap_max_strength,snap_gap_blend;
    uint32_t pip_coeff_valid[2][4]; float pip_a[2][4],pip_b[2][4];
} HexMocapUnityGestureSettings;
/* UTF8 JSON: schema kinds 1=Gesture v1..3, 2=WearRom v1,
 * 3=FourStep, 4=calibration bundle, 5=model driver snapshot, 6=legacy arc metadata.
 * Limit16MiB/depth64; never executes file text. */
HEX_GLOVE_API HexMocapUnityJsonHandle hex_mocap_unity_json_parse(const char* utf8, uint32_t bytes);
HEX_GLOVE_API void hex_mocap_unity_json_destroy(HexMocapUnityJsonHandle handle);
HEX_GLOVE_API int hex_mocap_unity_json_get_info(HexMocapUnityJsonHandle handle, HexMocapUnityJsonInfo* out);
HEX_GLOVE_API int hex_mocap_unity_json_get_contact(HexMocapUnityJsonHandle handle, int32_t hand, int32_t finger, HexMocapContactCalibration* calib, HexMocapJointAffineSet* maps, int32_t* preferred_style);
HEX_GLOVE_API int hex_mocap_unity_json_get_trajectory(HexMocapUnityJsonHandle handle, int32_t hand, int32_t finger, HexMocapContactSample* out, uint32_t capacity, uint32_t* required);
HEX_GLOVE_API int hex_mocap_unity_json_get_wear_rom(HexMocapUnityJsonHandle handle, int32_t hand, HexMocapUnityWearRom* out);
HEX_GLOVE_API int hex_mocap_unity_json_get_gesture_settings(HexMocapUnityJsonHandle handle, HexMocapUnityGestureSettings* out);
/* Calibration bundle measurements (thumb,index,middle,ring,little), metres. */
HEX_GLOVE_API int hex_mocap_unity_json_get_lengths(HexMocapUnityJsonHandle handle, int32_t hand, float measured[5], float model[5]);
/* Original FourStep JSON stores calibration scalars only, no pose snapshots. */
HEX_GLOVE_API int hex_mocap_unity_json_get_fourstep(HexMocapUnityJsonHandle handle, int32_t hand, HexFourStepCalibration* out);
/* Obsolete User JSON arc fields survive without a current source consumer.
 * Returns stored values verbatim; never reinterpret as phalanx/total lengths. */
HEX_GLOVE_API int hex_mocap_unity_json_get_legacy_arc(HexMocapUnityJsonHandle handle,float arc_index_middle_ring_little[4]);
HEX_GLOVE_API int hex_mocap_unity_json_get_node_offset(HexMocapUnityJsonHandle handle, uint32_t index, HexMocapUnityNodeOffset* out);
/* Read raw JSON subtree with slash-separated property names (empty=root).
 * Includes terminator in required; preserves unknown model driver fields. */
HEX_GLOVE_API int hex_mocap_unity_json_get_section(HexMocapUnityJsonHandle handle, const char* path, char* out_utf8, uint32_t capacity, uint32_t* required);

#ifdef __cplusplus
}
#endif

