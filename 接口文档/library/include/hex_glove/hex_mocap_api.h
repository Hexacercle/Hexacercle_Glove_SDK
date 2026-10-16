#pragma once
#include "hex_glove/hex_glove_api.h"
#include "hex_glove/mocap_model.h"
#include "hex_glove/mocap_driver.h"
#include "hex_glove/hex_angles_api.h"
#include "hex_glove/mocap_abi.h"
#include "hex_glove/mocap_calib_types.h"
#include "hex_glove/mocap_contact.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct HexMocapSession* HexMocapHandle;
HEX_GLOVE_API int hex_mocap_skeleton_definition_init(HexMocapSkeletonDefinition* definition, int hand);
HEX_GLOVE_API int hex_mocap_set_skeleton_definition(HexMocapHandle handle, int hand, const HexMocapSkeletonDefinition* definition);
HEX_GLOVE_API int hex_mocap_get_skeleton_definition(HexMocapHandle handle, int hand, HexMocapSkeletonDefinition* definition);
HEX_GLOVE_API int hex_mocap_get_display_pose(HexMocapHandle handle, int hand, HexMocapDisplayPose* pose);
HEX_GLOVE_API int hex_mocap_get_precise_angles(HexMocapHandle handle, int hand, HexAnglesSnapshot* angles);
HEX_GLOVE_API int hex_mocap_calibrate_angles(HexMocapHandle handle, int hand_mask);
HEX_GLOVE_API int hex_mocap_profile_load(HexMocapHandle handle, const char* path_utf8);
HEX_GLOVE_API int hex_mocap_start_r_capture(HexMocapHandle handle, int hand_mask, float seconds);
HEX_GLOVE_API int hex_mocap_get_r_capture_info(HexMocapHandle handle, HexMocapRCaptureInfo* info);
/* Seven-step calibration and per-hand contact capture used by the Qt integration sample. */
HEX_GLOVE_API int hex_mocap_seven_step_begin_original_ui(HexMocapHandle handle, int hand_mask);
HEX_GLOVE_API int hex_mocap_seven_step_cancel(HexMocapHandle handle);
HEX_GLOVE_API int hex_mocap_seven_step_get_state(HexMocapHandle handle, HexMocapSevenStepState* state);
HEX_GLOVE_API int hex_mocap_get_contact_capture_status(HexMocapHandle handle, int hand, HexMocapContactCaptureStatus* status);
HEX_GLOVE_API int hex_mocap_get_contact_quality(HexMocapHandle handle, int hand, HexMocapContactQuality* quality);
HEX_GLOVE_API int hex_mocap_contact_retry(HexMocapHandle handle, int hand_mask);
HEX_GLOVE_API HexMocapHandle hex_mocap_create(HexGloveHandle glove);
HEX_GLOVE_API void hex_mocap_destroy(HexMocapHandle handle);
HEX_GLOVE_API int hex_mocap_update(HexMocapHandle handle);
HEX_GLOVE_API uint32_t hex_mocap_get_api_version(void);
HEX_GLOVE_API uint32_t hex_mocap_get_status(HexMocapHandle handle);
HEX_GLOVE_API uint32_t hex_mocap_get_fresh_hand_mask(HexMocapHandle handle);
HEX_GLOVE_API uint32_t hex_mocap_get_capabilities(void);
HEX_GLOVE_API int hex_mocap_get_abi_info(HexMocapAbiInfo* info);
#ifdef __cplusplus
}
#endif
