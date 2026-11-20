#pragma once

#include "hex_glove/hex_glove_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public subset of the runtime hardware information ABI. */
typedef struct HexGloveHardwareInfo {
    uint32_t struct_size;
    int32_t valid, layout, glove_size, sensor_count, all_sys_mag_ready;
    uint32_t ready_sensor_mask;
    uint8_t hardware_major, hardware_minor, software_major, software_minor;
    char device_code[3];
    uint8_t sensor_sys[11], sensor_gyro[11], sensor_acc[11], sensor_mag[11];
    uint64_t revision;
} HexGloveHardwareInfo;

HEX_GLOVE_API int hex_glove_get_hardware_info(HexGloveHandle handle, int hand,
                                               HexGloveHardwareInfo* info);
/* Receiver-level six-digit ID. start() requests it automatically. */
HEX_GLOVE_API int hex_glove_request_device_id(HexGloveHandle handle);
HEX_GLOVE_API int hex_glove_get_device_id(HexGloveHandle handle, char* output,
                                           uint32_t capacity);

#ifdef __cplusplus
}
#endif
