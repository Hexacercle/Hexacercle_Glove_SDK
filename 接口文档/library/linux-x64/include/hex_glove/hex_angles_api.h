#pragma once
#include "hex_glove/hex_glove_api.h"
#include "hex_glove/mocap_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Curated external angle ABI; layouts match the SDK. */
/* Pipeline config uses this internal configuration type only through pointers. */
typedef struct HexAnglesConfig HexAnglesConfig;
typedef struct HexAnglesSnapshot {
    uint32_t struct_size, version, valid_mask, calibrated;
    int64_t timestamp_ms;
    uint32_t frame_sequence, flags; /* 1 wrist frozen, 2 realigning baseline */
    float finger[5][4]; /* actual cached values used by Unity hardware/UDP */
    float ui_finger[5][4]; /* four-finger negative floor; thumb PIP/DIP >=0 */
    HexMocapVec3 palm_twist_degrees;
    float thumb_twist_degrees, thumb_plane_degrees, thumb_hardware_smoothed_degrees;
    int32_t thumb_hardware_255;
    float thumb_flexion_readout_degrees; /* MCP + PIP * .5, as current Unity */
    float thumb_to_tip_mm[4], wrist_speed_deg_sec, wrist_smooth_blend;
    float finger_speed_deg_sec[5];
    uint32_t flex_hold_mask, spread_hold_mask, static_hold_mask, thumb_plane_valid;
} HexAnglesSnapshot;
enum HexAngleJointType { HEX_ANGLE_NONE=0, HEX_ANGLE_WRIST=1, HEX_ANGLE_THUMB_CMC=2,
    HEX_ANGLE_THUMB_MCP=3, HEX_ANGLE_THUMB_IP=4, HEX_ANGLE_MCP=5,
    HEX_ANGLE_PIP=6, HEX_ANGLE_DIP=7, HEX_ANGLE_TIP=8 };
typedef struct HexAngleJointDefinition {
    int32_t joint_type, enabled, enable_abduction, clamp, default_range;
    HexMocapVec3 flexion_axis, abduction_axis;
    float flexion_min, flexion_max, abduction_min, abduction_max;
} HexAngleJointDefinition;
typedef struct HexAngleHubConfig {
    uint32_t struct_size, version;
    HexAngleJointDefinition joint[21];
    HexMocapVec3 mcp_plane_normal_local, spread_bone_direction_local, spread_palm_normal_local;
    int32_t mcp_absolute, sequential_twist, smooth_output;
    float smooth_speed;
    int32_t spread_mode; /* 0 signed projected middle,1 adjacent3D,2 palm->tip */
    int32_t spread_axis_mode; /* 0 bone local direction,1 anchor->tip */
    int32_t spread_normal_source; /* 0 palm local normal,1 cross(index,middle) */
    int32_t spread_anchor_offset; /* 0,1,2 in each finger chain */
} HexAngleHubConfig;
typedef struct HexAngleHubState {
    uint32_t struct_size, version, calibrated, valid_mask;
    HexQuat neutral_rotation[21];
    HexMocapVec3 neutral_outgoing_world[21], neutral_outgoing_palm[21];
    float neutral_flexion[21], flexion[21], abduction[21], abduction_geometric[21];
    float spread[5], thumb_elevation, thumb_vs_index, thumb_nail_to_palm;
    uint32_t spread_valid, thumb_aux_valid;
} HexAngleHubState;
HEX_GLOVE_API int hex_angle_hub_config_init(HexAngleHubConfig* config);
HEX_GLOVE_API int hex_angle_hub_state_init(HexAngleHubState* state);
HEX_GLOVE_API int hex_angle_hub_calibrate(const HexAngleHubConfig* config, HexAngleHubState* state, const HexMocapDisplayPose* pose, const HexMocapVec3* camera_forward);
HEX_GLOVE_API int hex_angle_hub_update(const HexAngleHubConfig* config, HexAngleHubState* state, const HexMocapDisplayPose* pose, float dt_seconds, const HexMocapVec3* camera_forward, const HexMocapVec3* nail_world_normal);
#ifdef __cplusplus
}
#endif
