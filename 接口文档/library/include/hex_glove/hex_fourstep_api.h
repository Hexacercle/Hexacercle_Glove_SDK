#pragma once
#include "hex_glove/hex_glove_api.h"
#include "hex_glove/mocap_model.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct HexFourStepCalibration {
    uint32_t struct_size,version,steps; /* bits0..3: captured1..4 */
    float zero_pitch[5],zero_yaw[5],spread_yaw[5],bend_pitch[5],thumb_adduct_yaw;
} HexFourStepCalibration;
typedef struct HexFourStepPercent {
    uint32_t struct_size;
    float bend[5],spread[5],thumb_adduct; /* 0..100, -1 unavailable/range<5deg */
} HexFourStepPercent;
typedef struct HexFourStepSnapshots {
    uint32_t struct_size,version,steps;
    HexQuat local_rotation[4][21]; /* straight,spread,fist,thumb combined */
} HexFourStepSnapshots;
typedef struct HexFourStepDriveConfig {
    uint32_t struct_size,version;
    int32_t mode; /* 0 PureTestDriver max(bend,add),1 SkeletonApplier sequential */
    float pip_follow,spread_weight,thumb_abduct_weight,thumb_adduct_weight;
} HexFourStepDriveConfig;
HEX_GLOVE_API int hex_fourstep_calibration_init(HexFourStepCalibration* calibration);
HEX_GLOVE_API int hex_fourstep_snapshots_init(HexFourStepSnapshots* snapshots);
HEX_GLOVE_API int hex_fourstep_drive_config_init(HexFourStepDriveConfig* config);
/* Same captured pitch/yaw data used by the original GetGloveLeft/Right API.
 * Step1 invalidates2..4. Steps2..4 require1 and may be repeated independently.
 * R's time window is owned by mocap_start_r_capture; capture1 again at completion.
 */
HEX_GLOVE_API int hex_fourstep_capture(HexFourStepCalibration* calibration,int step,const float* pitch5,const float* yaw5);
HEX_GLOVE_API int hex_fourstep_evaluate(const HexFourStepCalibration* calibration,const float* pitch5,const float* yaw5,HexFourStepPercent* percent);
HEX_GLOVE_API int hex_fourstep_capture_pose(HexFourStepSnapshots* snapshots,int step,const HexMocapDisplayPose* pose);
/* Keeps the incoming palm/tip offsets, writes saved finger local rotations and
 * recomputes full FK. A preview/test alternative to the main sensor driver. */
HEX_GLOVE_API int hex_fourstep_apply(const HexFourStepDriveConfig* config,const HexFourStepSnapshots* snapshots,const HexFourStepPercent* percent,const HexMocapDisplayPose* base_pose,HexMocapDisplayPose* out_pose);
HEX_GLOVE_API float hex_fourstep_axis_percent(float relative_degrees,float endpoint_degrees);
HEX_GLOVE_API float hex_fourstep_bend_percent(float amount_degrees,float range_degrees);
#ifdef __cplusplus
}
#endif
