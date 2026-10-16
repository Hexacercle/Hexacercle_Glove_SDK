#pragma once

#include "hex_glove/types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Hand side (matches Unity DexterousHandJointAngleHub.HandSide). */
typedef enum HexMocapHandSide {
    HEX_MOCAP_HAND_LEFT = 0,
    HEX_MOCAP_HAND_RIGHT = 1
} HexMocapHandSide;

/**
 * Per-hand joint slots for S0/S1 skeleton pose.
 * Naming aligns with Unity MHandAngleTest Real* bones (see joint_bind_table.h / docs).
 */
typedef enum HexMocapJointId {
    HEX_MOCAP_JOINT_PALM = 0,       /**< RealHand */
    HEX_MOCAP_JOINT_THUMB_CMC = 1,  /**< RealThumb */
    HEX_MOCAP_JOINT_THUMB_MCP = 2,  /**< RealThumb_2 */
    HEX_MOCAP_JOINT_THUMB_IP = 3,   /**< RealThumb_3 */
    HEX_MOCAP_JOINT_INDEX_MCP = 4,  /**< RealIndex_1 */
    HEX_MOCAP_JOINT_INDEX_PIP = 5,  /**< RealIndex_2 */
    HEX_MOCAP_JOINT_INDEX_DIP = 6,  /**< RealIndex_3 */
    HEX_MOCAP_JOINT_MIDDLE_MCP = 7, /**< RealMiddle_1 */
    HEX_MOCAP_JOINT_MIDDLE_PIP = 8, /**< RealMiddle_2 */
    HEX_MOCAP_JOINT_MIDDLE_DIP = 9, /**< RealMiddle_3 */
    HEX_MOCAP_JOINT_RING_MCP = 10,  /**< RealRing_1 */
    HEX_MOCAP_JOINT_RING_PIP = 11,  /**< RealRing_2 */
    HEX_MOCAP_JOINT_RING_DIP = 12,  /**< RealRing_3 */
    HEX_MOCAP_JOINT_LITTLE_MCP = 13,/**< RealPinky_1 */
    HEX_MOCAP_JOINT_LITTLE_PIP = 14,/**< RealPinky_2 */
    HEX_MOCAP_JOINT_LITTLE_DIP = 15,/**< RealPinky_3 */
    HEX_MOCAP_JOINT_COUNT = 16
} HexMocapJointId;

/** Bit mask for R-calibration / sampling APIs. */
typedef enum HexMocapHandMask {
    HEX_MOCAP_MASK_NONE = 0,
    HEX_MOCAP_MASK_LEFT = 1,
    HEX_MOCAP_MASK_RIGHT = 2,
    HEX_MOCAP_MASK_BOTH = 3
} HexMocapHandMask;

/** Solver status flags (bitfield). */
typedef enum HexMocapStatusFlags {
    HEX_MOCAP_STATUS_OK = 0,
    HEX_MOCAP_STATUS_STUB = 1 << 0,           /**< S0 empty implementation */
    HEX_MOCAP_STATUS_NO_GLOVE = 1 << 1,
    HEX_MOCAP_STATUS_NO_SYNC_FRAME = 1 << 2,
    HEX_MOCAP_STATUS_R_NOT_DONE = 1 << 3,
    HEX_MOCAP_STATUS_NOT_IMPLEMENTED = 1 << 4
} HexMocapStatusFlags;

/**
 * Unity WORLD target rotations. The legacy field name is retained for ABI
 * compatibility. To display: local = inverse(parentWorld) * targetWorld.
 */
typedef struct HexHandSkeletonPose {
    HexQuat local_rotation[HEX_MOCAP_JOINT_COUNT];
    uint32_t valid_mask; /**< bit i set => local_rotation[i] is valid */
} HexHandSkeletonPose;

typedef struct HexSkeletonPose {
    int64_t timestamp_ms; /**< Live: host steady clock solve time, not UTC/device time. Offline: caller timeline. */
    uint8_t frame_sequence;
    HexHandSkeletonPose left;
    HexHandSkeletonPose right;
    uint32_t status_flags;
} HexSkeletonPose;

/**
 * Flexion / abduction (degrees) per joint slot.
 * Convention (S1): relative to R-neutral; sign matches Unity PreciseHandAngle / Hub where possible.
 */
typedef struct HexHandJointAngles {
    float flexion_deg[HEX_MOCAP_JOINT_COUNT];
    float abduction_deg[HEX_MOCAP_JOINT_COUNT];
    uint32_t valid_mask; /**< Legacy four-finger bits 4..15; current driver adds thumb diagnostics 1..3. Precise geometry uses HexAnglesSnapshot. */
} HexHandJointAngles;

typedef struct HexJointAngles {
    int64_t timestamp_ms;
    HexHandJointAngles left;
    HexHandJointAngles right;
    uint32_t status_flags;
} HexJointAngles;

#ifdef __cplusplus
}
#endif
