#pragma once

#include "hex_glove/types.h"

#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#  ifdef HEX_GLOVE_EXPORTS
#    define HEX_GLOVE_API __declspec(dllexport)
#  else
#    define HEX_GLOVE_API __declspec(dllimport)
#  endif
#else
#  if defined(HEX_GLOVE_EXPORTS)
#    define HEX_GLOVE_API __attribute__((visibility("default")))
#  else
#    define HEX_GLOVE_API
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HexGloveSession* HexGloveHandle;

HEX_GLOVE_API int hex_glove_get_raw_hand_frame(HexGloveHandle handle, int hand, HexGloveRawHandFrame* out_frame);
HEX_GLOVE_API int hex_glove_get_receive_diagnostics(HexGloveHandle handle, HexGloveReceiveDiagnostics* out);

HEX_GLOVE_API HexGloveHandle hex_glove_create(void);
HEX_GLOVE_API void hex_glove_destroy(HexGloveHandle handle);

HEX_GLOVE_API int hex_glove_start(HexGloveHandle handle, const char* port_name);
/* Last start failure on this handle. stage: 0=none, 1=invalid port,
 * 2=OS open, 3=serial configuration, 4=start command write.
 * platform_code is GetLastError on Windows or errno on Linux; 0 if unavailable. */
HEX_GLOVE_API int hex_glove_get_last_open_error(HexGloveHandle handle,
                                                uint32_t* stage, uint32_t* platform_code);
HEX_GLOVE_API void hex_glove_shutdown(HexGloveHandle handle);
HEX_GLOVE_API void hex_glove_abort(HexGloveHandle handle);

HEX_GLOVE_API void hex_glove_send_get_route(HexGloveHandle handle);
HEX_GLOVE_API int hex_glove_is_route_received(HexGloveHandle handle);
HEX_GLOVE_API int hex_glove_get_route(HexGloveHandle handle);

HEX_GLOVE_API int hex_glove_is_left_enabled(HexGloveHandle handle);
HEX_GLOVE_API int hex_glove_is_right_enabled(HexGloveHandle handle);
HEX_GLOVE_API int hex_glove_is_left_new_node_enabled(HexGloveHandle handle);
HEX_GLOVE_API int hex_glove_is_right_new_node_enabled(HexGloveHandle handle);

/** Write 6 quaternions into out_quats[6]. Returns 1 on success. */
HEX_GLOVE_API int hex_glove_get_waist_left(HexGloveHandle handle, HexQuat* out_quats);
HEX_GLOVE_API int hex_glove_get_waist_right(HexGloveHandle handle, HexQuat* out_quats);

/** Optional hardware check bytes (18). Pass NULL to skip. */
HEX_GLOVE_API int hex_glove_get_waist_left_ex(HexGloveHandle handle, HexQuat* out_quats, uint8_t* hardware_check18);
HEX_GLOVE_API int hex_glove_get_waist_right_ex(HexGloveHandle handle, HexQuat* out_quats, uint8_t* hardware_check18);

/** pitch[5], yaw[5] relative to palm (finger 0). */
HEX_GLOVE_API int hex_glove_get_glove_left(HexGloveHandle handle, double* out_pitch5, double* out_yaw5);
HEX_GLOVE_API int hex_glove_get_glove_right(HexGloveHandle handle, double* out_pitch5, double* out_yaw5);

HEX_GLOVE_API int hex_glove_get_new_node_left(HexGloveHandle handle, double* out_pitch5, double* out_yaw5);
HEX_GLOVE_API int hex_glove_get_new_node_right(HexGloveHandle handle, double* out_pitch5, double* out_yaw5);

HEX_GLOVE_API int hex_glove_get_new_node_quat_left(HexGloveHandle handle, HexQuat* out_quats);
HEX_GLOVE_API int hex_glove_get_new_node_quat_right(HexGloveHandle handle, HexQuat* out_quats);

HEX_GLOVE_API double hex_glove_get_batt_left(HexGloveHandle handle);
HEX_GLOVE_API double hex_glove_get_batt_right(HexGloveHandle handle);

HEX_GLOVE_API HexGloveImuLayout hex_glove_get_imu_layout_left(HexGloveHandle handle);
HEX_GLOVE_API HexGloveImuLayout hex_glove_get_imu_layout_right(HexGloveHandle handle);

/** Copy HardwareCheck 18 bytes. hand: 0=left 1=right. Returns 1 if data present. */
HEX_GLOVE_API int hex_glove_get_hardware_check(HexGloveHandle handle, int hand, uint8_t out18[HEX_GLOVE_HW_CHECK_BYTES]);

/** Fill device meta snapshot (layout/product/route/battery/hwcheck). Returns 1 on success. */
HEX_GLOVE_API int hex_glove_get_device_snapshot(HexGloveHandle handle, HexGloveDeviceSnapshot* out);

HEX_GLOVE_API uint8_t hex_glove_get_frame_seq_left(HexGloveHandle handle);
HEX_GLOVE_API uint8_t hex_glove_get_frame_seq_right(HexGloveHandle handle);

HEX_GLOVE_API int hex_glove_try_dequeue_synced(HexGloveHandle handle, HexSyncedGloveFrame* out_frame);
HEX_GLOVE_API HexSyncedGloveStats hex_glove_get_synced_stats(HexGloveHandle handle);

/** Debug: total raw serial bytes received since start. */
HEX_GLOVE_API uint64_t hex_glove_get_rx_bytes(HexGloveHandle handle);

/**
 * Thumb Unity alignment. current_quat is Unity thumb pose; original_quat receives raw sensor.
 * Returns aligned Unity quaternion.
 */
HEX_GLOVE_API HexQuat hex_glove_get_thumb_left(HexGloveHandle handle, HexQuat current_quat, HexQuat* original_quat);
HEX_GLOVE_API HexQuat hex_glove_get_thumb_right(HexGloveHandle handle, HexQuat current_quat, HexQuat* original_quat);

#ifdef __cplusplus
}
#endif
