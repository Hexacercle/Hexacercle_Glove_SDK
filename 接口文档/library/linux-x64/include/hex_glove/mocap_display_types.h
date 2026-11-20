#pragma once

/**
 * Window / display-hand joint slots (kulou_shou_New_窗口显示 / hc_*).
 * Per hand: palm + thumb×4 + four fingers ×4 = 21.
 * Recorder numeric ids (DexterousHandModelConfig name): L=1..21, R=22..42.
 * Do not renumber without a major mocap API bump.
 */

#include "hex_glove/mocap_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HexMocapDisplayJointId {
    HEX_MOCAP_DISP_PALM = 0,
    HEX_MOCAP_DISP_THUMB_1 = 1,
    HEX_MOCAP_DISP_THUMB_2 = 2,
    HEX_MOCAP_DISP_THUMB_3 = 3,
    HEX_MOCAP_DISP_THUMB_4 = 4,
    HEX_MOCAP_DISP_INDEX_1 = 5,
    HEX_MOCAP_DISP_INDEX_2 = 6,
    HEX_MOCAP_DISP_INDEX_3 = 7,
    HEX_MOCAP_DISP_INDEX_4 = 8,
    HEX_MOCAP_DISP_MIDDLE_1 = 9,
    HEX_MOCAP_DISP_MIDDLE_2 = 10,
    HEX_MOCAP_DISP_MIDDLE_3 = 11,
    HEX_MOCAP_DISP_MIDDLE_4 = 12,
    HEX_MOCAP_DISP_RING_1 = 13,
    HEX_MOCAP_DISP_RING_2 = 14,
    HEX_MOCAP_DISP_RING_3 = 15,
    HEX_MOCAP_DISP_RING_4 = 16,
    HEX_MOCAP_DISP_LITTLE_1 = 17,
    HEX_MOCAP_DISP_LITTLE_2 = 18,
    HEX_MOCAP_DISP_LITTLE_3 = 19,
    HEX_MOCAP_DISP_LITTLE_4 = 20,
    HEX_MOCAP_DISP_JOINT_COUNT = 21
} HexMocapDisplayJointId;

#ifdef __cplusplus
}
#endif
