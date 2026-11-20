#pragma once
#include "hex_glove/hex_mocap_unity_extensions.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct HexModelNodeConfig {
    int32_t source_slot, derivative_slot; /* canonical same-hand 0..20, -1 unavailable */
    uint32_t rotation_mode, sync_axis_mask, preserve_offset, preserve_scale;
    uint32_t drive_scale, override_scale;
    HexMocapVec3 manual_euler,offset_euler,bone_scale,post_offset,custom_scale;
    float pip_dip_k;
} HexModelNodeConfig;
typedef struct HexModelDriverConfig {
    uint32_t struct_size,version;
    int32_t hand; /* 0 left,1 right */
    HexModelNodeConfig node[21];
    float thumb_second_multiplier,second_multiplier[4],pip_dip_k,biomechanical_sign;
    float seven_pip_multiplier,seven_dip_multiplier,seven_second_sign,seven_distal_sign;
    float seven_deadzone,seven_saturation,seven_tip_gain,seven_axis_floor;
    HexMocapVec3 thumb_euler_offset;
    uint32_t unwrap_euler,thumb_biomechanical_gain,smooth_rotation,use_deadzone;
    float rotation_lerp_speed,deadzone_angle;
    float thumb_max_euler,thumb_gain_min,thumb_gain_max,thumb_edge0,thumb_edge1;
    HexMocapVec3 thumb_test_extra,index2_test_extra;
    uint32_t thumb_test_axis_mask,index2_test_axis_mask,thumb_distance_y;
    float thumb_distance_baseline,thumb_distance_near,thumb_y_near,thumb_y_far,thumb_y_multiplier;
} HexModelDriverConfig;
typedef struct HexModelDriverFrame {
    uint32_t struct_size;
    float dt_seconds;
    uint32_t source_valid_mask,tip_routed_to_pip,disable_pip,ip_hold_mask,pose_formula_mask,replay_active;
    float formula_pip_deg[4],formula_weight[4],thumb_distance_m;
    uint32_t runtime_offset_mask;
    HexMocapVec3 runtime_offset[21]; /* transient node offsets; config is unchanged */
} HexModelDriverFrame;
typedef struct HexModelDriverSession* HexModelDriverHandle;
HEX_GLOVE_API int hex_model_driver_config_init(HexModelDriverConfig* config,int32_t hand);
/* Current ModelConfigFile schema1/2. Applies only this hand's numeric/named IDs.
 * reset to defaults with init first; import merges named fields and matching nodes. */
HEX_GLOVE_API int hex_model_driver_config_import_json(HexModelDriverConfig* config,const char* utf8,uint32_t bytes);
HEX_GLOVE_API HexModelDriverHandle hex_model_driver_create(const HexModelDriverConfig* config,const HexMocapDisplayPose* initial_model);
HEX_GLOVE_API HexModelDriverHandle hex_model_driver_clone(HexModelDriverHandle handle);
HEX_GLOVE_API void hex_model_driver_destroy(HexModelDriverHandle handle);
HEX_GLOVE_API int hex_model_driver_reset(HexModelDriverHandle handle);
HEX_GLOVE_API int hex_model_driver_set_config(HexModelDriverHandle handle,const HexModelDriverConfig* config);
HEX_GLOVE_API int hex_model_driver_get_config(HexModelDriverHandle handle,HexModelDriverConfig* out);
/* source = recorder's local+world pose; output model hierarchy remains its own.
 * frame flags mirror runtime MHand gates. One call per render/solve tick. */
HEX_GLOVE_API int hex_model_driver_process(HexModelDriverHandle handle,const HexMocapDisplayPose* source,const HexModelDriverFrame* frame,HexMocapDisplayPose* out);
/* Current neutral calibration: axes preserve flags and Direct quaternion delta. */
HEX_GLOVE_API int hex_model_driver_capture_neutral(HexModelDriverConfig* config,const HexMocapDisplayPose* source,const HexMocapDisplayPose* desired);
#ifdef __cplusplus
}
#endif
