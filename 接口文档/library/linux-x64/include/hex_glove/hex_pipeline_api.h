#pragma once
#include "hex_glove/hex_mocap_api.h"
#include "hex_glove/hex_model_driver_api.h"
#include "hex_glove/hex_angles_api.h"
#include "hex_glove/hex_fourstep_api.h"
#include "hex_glove/hex_runtime_offsets_api.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct HexMocapPipelineConfig {
    uint32_t struct_size,version;
    int32_t model_driver_enabled,ccd_enabled,fourstep_enabled;
    HexModelDriverConfig model_driver;
    HexMocapCcdConfig ccd;
    HexFourStepDriveConfig fourstep;
} HexMocapPipelineConfig;
enum HexMocapPipelineStage {
    HEX_PIPELINE_MODEL_DRIVER=1, HEX_PIPELINE_FOURSTEP=2, HEX_PIPELINE_CCD=4,
    HEX_PIPELINE_PRECISE_ANGLES=8, HEX_PIPELINE_RUNTIME_OFFSETS=16,
    HEX_PIPELINE_CONTACT_REPLAY=32
};
typedef struct HexMocapPipelineState {
    uint32_t struct_size,version,ready,applied_mask,failed_mask;
    int64_t timestamp_ms;
    uint32_t frame_sequence;
} HexMocapPipelineState;
/* Unity ModelDriver and post-sync contact are optional until configured. init
 * enables both, with PureTest FourStep disabled. Upload the display model first.
 * For current scene load its saved ModelDriver JSON into config.model_driver.
 */
HEX_GLOVE_API int hex_mocap_pipeline_config_init(HexMocapPipelineConfig* config,int hand);
/* Recorder source and display target may be different models. Set the target
 * with set_skeleton_definition, then the source here. This derives source bind
 * and PIP neutrals and invalidates R. No source keeps the legacy one-model path. */
HEX_GLOVE_API int hex_mocap_set_source_skeleton_definition(HexMocapHandle handle,int hand,const HexMocapSkeletonDefinition* definition);
HEX_GLOVE_API int hex_mocap_get_source_skeleton_definition(HexMocapHandle handle,int hand,HexMocapSkeletonDefinition* definition);
HEX_GLOVE_API int hex_mocap_set_pipeline_config(HexMocapHandle handle,int hand,const HexMocapPipelineConfig* config);
HEX_GLOVE_API int hex_mocap_get_pipeline_config(HexMocapHandle handle,int hand,HexMocapPipelineConfig* config);
/* ready=0 after model/configuration changes, until a successful input update.
 * failed_mask identifies a rejected stage; never treat it as verified final data. */
HEX_GLOVE_API int hex_mocap_get_pipeline_state(HexMocapHandle handle,int hand,HexMocapPipelineState* state);
/* Session-only diagnostic view: preserve sensor/model mapping, bypass contact
 * formula, runtime offsets, FourStep, contact replay, 825 and CCD. Does not
 * erase calibration. Reject while calibration is collecting. Not persisted. */
HEX_GLOVE_API int hex_mocap_set_diagnostic_baseline(HexMocapHandle handle,int hand,int enabled);
/* Offline right-index candidate only. Explicit points are in thumb IP (3) and
 * index DIP (7) local model coordinates. Pad and tip require distinct measured
 * points. Never selects a gesture or attracts a live finger. Return 1=accepted,
 * 2=outside bounded correction (out is unchanged base), 0=invalid arguments.
 * Only joints 1,2,3,5,6,7 may rotate; distal finger joints use flexion X only.
 * limit is cumulative displacement from base per joint, [0,12] degrees. */
HEX_GLOVE_API int hex_mocap_contact_point_candidate(const HexMocapDisplayPose* base,
    HexMocapVec3 thumb_point,HexMocapVec3 index_point,float limit_deg,
    float tolerance_m,HexMocapDisplayPose* out,float* residual_m);
HEX_GLOVE_API int hex_mocap_get_ccd_state(HexMocapHandle handle,int hand,HexMocapCcdState* state);
HEX_GLOVE_API int hex_mocap_set_runtime_offsets_config(HexMocapHandle handle,int hand,const HexRuntimeOffsetsConfig* config);
HEX_GLOVE_API int hex_mocap_get_runtime_offsets_config(HexMocapHandle handle,int hand,HexRuntimeOffsetsConfig* config);
HEX_GLOVE_API int hex_mocap_get_runtime_offsets_state(HexMocapHandle handle,int hand,HexRuntimeOffsetsState* state);
/* get_display_pose now contains final SDK model mapping/CCD when enabled;
 * get_driver_display_pose exposes the pre-mapping FK for diagnostic comparison.
 * Reading either API is pure: repeating a getter never advances smoothing.
 */
HEX_GLOVE_API int hex_mocap_get_driver_display_pose(HexMocapHandle handle,int hand,HexMocapDisplayPose* pose);
HEX_GLOVE_API int hex_mocap_get_precise_angles(HexMocapHandle handle,int hand,HexAnglesSnapshot* angles);
HEX_GLOVE_API int hex_mocap_set_angles_config(HexMocapHandle handle,const HexAnglesConfig* config);
HEX_GLOVE_API int hex_mocap_get_angles_config(HexMocapHandle handle,HexAnglesConfig* config);
HEX_GLOVE_API int hex_mocap_calibrate_angles(HexMocapHandle handle,int hand_mask);
/* Capture current raw-sensor pitch/yaw and final pose. Step1 can be repeated
 * after the R window; no device command is emitted by this API. */
HEX_GLOVE_API int hex_mocap_capture_fourstep(HexMocapHandle handle,int hand,int step);
HEX_GLOVE_API int hex_mocap_get_fourstep(HexMocapHandle handle,int hand,HexFourStepCalibration* calibration,HexFourStepSnapshots* snapshots);
HEX_GLOVE_API int hex_mocap_get_fourstep_percent(HexMocapHandle handle,int hand,HexFourStepPercent* percent);
/* Decodes and atomically applies supported legacy profile/bundle/model fields.
 * No file is executed. R sensor baselines absent from old JSON still need R.
 */
HEX_GLOVE_API int hex_mocap_import_unity_json(HexMocapHandle handle,const char* utf8,uint32_t bytes);
HEX_GLOVE_API int hex_mocap_load_unity_json(HexMocapHandle handle,const char* utf8_path);
#ifdef __cplusplus
}
#endif
