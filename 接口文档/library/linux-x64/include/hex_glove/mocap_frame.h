#pragma once
#include "hex_glove/types.h"

/** Versioned offline input. Sensor coordinates, xyzw (not Unity coordinates).
 * Root: palm, thumb root, index, middle, ring, little.
 * Tip: thumb tip, unused, index, middle, ring, little.
 * V1: all roots required; M7 tips ignored (legacy synthetic path).
 * V2: both layouts require tip_valid and tip slots 0,2,3,4,5. M7 roots 2..5
 * whose squared norm is below .95 (or nonfinite) fall back to the same tip slot.
 * V2 layout changes/disconnect clear runtime R; config/bind/G remain.
 * Unused tip slot 1 is ignored. V2 is required for current real M7 packets.
 */
typedef struct HexMocapInputHand {
    int32_t present;
    int32_t layout;
    int32_t tip_valid;
    HexQuat root[6];
    HexQuat tip[6];
} HexMocapInputHand;

typedef struct HexMocapInputFrame {
    uint32_t struct_size;
    uint32_t version;           /**< 1=legacy; 2=current raw M7/M11 */
    int64_t timestamp_ms;        /**< Caller timeline, nonnegative, strictly increasing. */
    float dt_seconds;           /**< finite, (0, 1] */
    uint32_t frame_sequence;    /**< low 8 bits used by legacy pose */
    HexMocapInputHand left;
    HexMocapInputHand right;
} HexMocapInputFrame;
