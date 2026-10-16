#pragma once

#include "hex_glove/mocap_display_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HexMocapVec3 { float x, y, z; } HexMocapVec3;

/** 21 display slots, canonical palm + five 4-joint chains. Metres, Unity axes.
 * Matrices are column-major. Root is the external parent of slot 0.
 * init creates a synthetic hand for diagnostics; upload model data for parity.
 */
typedef struct HexMocapSkeletonDefinition {
    uint32_t struct_size;
    uint32_t version;
    int32_t parent[21];
    HexMocapVec3 rest_local_position[21];
    HexQuat rest_local_rotation[21];
    HexMocapVec3 rest_local_scale[21];
    HexMocapVec3 root_position;
    HexQuat root_rotation;
    HexMocapVec3 root_scale;
} HexMocapSkeletonDefinition;

/** Geometry changes use child offsets, never repeated per-bone scale factors.
 * Finger order thumb,index,middle,ring,little. All dimensions relative to the
 * uploaded unmodified rest model. Reapplying is idempotent.
 */
typedef struct HexMocapHandShape {
    uint32_t struct_size;
    uint32_t version;
    float finger_length_scale[5];
    float palm_length_scale;
    float palm_width_scale;
    float overall_scale;
    int32_t use_golden_ratio;
    float golden_ratio;
    int32_t use_arch;
    float finger_vs_middle[5];
    int32_t use_calibrated_lengths;
} HexMocapHandShape;

/** Complete engine-free FK. valid_mask marks defined bones, tracked_mask marks
 * live solved bones (zero for rest preview / dropped input). Return success
 * for a configured model even when tracking is unavailable; inspect masks.
 */
typedef struct HexMocapDisplayPose {
    uint32_t struct_size;
    uint32_t version;
    int64_t timestamp_ms;
    uint32_t frame_sequence;
    uint32_t valid_mask;
    uint32_t tracked_mask;
    uint32_t status_flags;
    HexMocapVec3 world_position[21];
    HexQuat world_rotation[21];
    HexMocapVec3 local_position[21];
    HexQuat local_rotation[21];
    HexMocapVec3 local_scale[21];
    HexMocapVec3 world_scale[21];
    float world_matrix[21][16];
} HexMocapDisplayPose;

#ifdef __cplusplus
}
#endif
