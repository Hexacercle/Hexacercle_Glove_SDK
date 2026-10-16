#pragma once

/**
 * Frozen joint bind table: HexMocapJointId <-> Unity Real* / NodeRecorder ids,
 * plus HexMocapDisplayJointId <-> hc_* / Recorder numeric ids.
 * Do not renumber without a major API version bump.
 */

#include "hex_glove/mocap_types.h"
#include "hex_glove/mocap_display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Unity MHandAngleTest field name for right-hand Real* (left uses same names on MHandAngleLeftHand). */
static inline const char* hex_mocap_joint_unity_real_name(HexMocapJointId id)
{
    switch (id) {
    case HEX_MOCAP_JOINT_PALM: return "RealHand";
    case HEX_MOCAP_JOINT_THUMB_CMC: return "RealThumb";
    case HEX_MOCAP_JOINT_THUMB_MCP: return "RealThumb_2";
    case HEX_MOCAP_JOINT_THUMB_IP: return "RealThumb_3";
    case HEX_MOCAP_JOINT_INDEX_MCP: return "RealIndex_1";
    case HEX_MOCAP_JOINT_INDEX_PIP: return "RealIndex_2";
    case HEX_MOCAP_JOINT_INDEX_DIP: return "RealIndex_3";
    case HEX_MOCAP_JOINT_MIDDLE_MCP: return "RealMiddle_1";
    case HEX_MOCAP_JOINT_MIDDLE_PIP: return "RealMiddle_2";
    case HEX_MOCAP_JOINT_MIDDLE_DIP: return "RealMiddle_3";
    case HEX_MOCAP_JOINT_RING_MCP: return "RealRing_1";
    case HEX_MOCAP_JOINT_RING_PIP: return "RealRing_2";
    case HEX_MOCAP_JOINT_RING_DIP: return "RealRing_3";
    case HEX_MOCAP_JOINT_LITTLE_MCP: return "RealPinky_1";
    case HEX_MOCAP_JOINT_LITTLE_PIP: return "RealPinky_2";
    case HEX_MOCAP_JOINT_LITTLE_DIP: return "RealPinky_3";
    default: return "";
    }
}

/** Stable string id for JSON / logging (snake_case). */
static inline const char* hex_mocap_joint_stable_id(HexMocapJointId id)
{
    switch (id) {
    case HEX_MOCAP_JOINT_PALM: return "palm";
    case HEX_MOCAP_JOINT_THUMB_CMC: return "thumb_cmc";
    case HEX_MOCAP_JOINT_THUMB_MCP: return "thumb_mcp";
    case HEX_MOCAP_JOINT_THUMB_IP: return "thumb_ip";
    case HEX_MOCAP_JOINT_INDEX_MCP: return "index_mcp";
    case HEX_MOCAP_JOINT_INDEX_PIP: return "index_pip";
    case HEX_MOCAP_JOINT_INDEX_DIP: return "index_dip";
    case HEX_MOCAP_JOINT_MIDDLE_MCP: return "middle_mcp";
    case HEX_MOCAP_JOINT_MIDDLE_PIP: return "middle_pip";
    case HEX_MOCAP_JOINT_MIDDLE_DIP: return "middle_dip";
    case HEX_MOCAP_JOINT_RING_MCP: return "ring_mcp";
    case HEX_MOCAP_JOINT_RING_PIP: return "ring_pip";
    case HEX_MOCAP_JOINT_RING_DIP: return "ring_dip";
    case HEX_MOCAP_JOINT_LITTLE_MCP: return "little_mcp";
    case HEX_MOCAP_JOINT_LITTLE_PIP: return "little_pip";
    case HEX_MOCAP_JOINT_LITTLE_DIP: return "little_dip";
    default: return "";
    }
}

/** Window Transform note / hc_* name (right-hand suffix; left uses _L). */
static inline const char* hex_mocap_display_joint_hc_stem(HexMocapDisplayJointId id)
{
    switch (id) {
    case HEX_MOCAP_DISP_PALM: return "hc_Hand";
    case HEX_MOCAP_DISP_THUMB_1: return "hc_Thumb1";
    case HEX_MOCAP_DISP_THUMB_2: return "hc_Thumb2";
    case HEX_MOCAP_DISP_THUMB_3: return "hc_Thumb3";
    case HEX_MOCAP_DISP_THUMB_4: return "hc_Thumb4";
    case HEX_MOCAP_DISP_INDEX_1: return "hc_Index1";
    case HEX_MOCAP_DISP_INDEX_2: return "hc_Index2";
    case HEX_MOCAP_DISP_INDEX_3: return "hc_Index3";
    case HEX_MOCAP_DISP_INDEX_4: return "hc_Index4";
    case HEX_MOCAP_DISP_MIDDLE_1: return "hc_Middle1";
    case HEX_MOCAP_DISP_MIDDLE_2: return "hc_Middle2";
    case HEX_MOCAP_DISP_MIDDLE_3: return "hc_Middle3";
    case HEX_MOCAP_DISP_MIDDLE_4: return "hc_Middle4";
    case HEX_MOCAP_DISP_RING_1: return "hc_Ring1"; /* left scene may use hc_Ling* */
    case HEX_MOCAP_DISP_RING_2: return "hc_Ring2";
    case HEX_MOCAP_DISP_RING_3: return "hc_Ring3";
    case HEX_MOCAP_DISP_RING_4: return "hc_Ring4";
    case HEX_MOCAP_DISP_LITTLE_1: return "hc_Pinky1";
    case HEX_MOCAP_DISP_LITTLE_2: return "hc_Pinky2";
    case HEX_MOCAP_DISP_LITTLE_3: return "hc_Pinky3";
    case HEX_MOCAP_DISP_LITTLE_4: return "hc_Pinky4";
    default: return "";
    }
}

/**
 * Map display slot → Real* HexMocapJointId, or -1 if display-only (e.g. *4 tip mesh).
 */
static inline int hex_mocap_display_slot_to_real(int display_slot)
{
    switch (display_slot) {
    case HEX_MOCAP_DISP_PALM: return HEX_MOCAP_JOINT_PALM;
    case HEX_MOCAP_DISP_THUMB_1: return HEX_MOCAP_JOINT_THUMB_CMC;
    case HEX_MOCAP_DISP_THUMB_2: return HEX_MOCAP_JOINT_THUMB_MCP;
    case HEX_MOCAP_DISP_THUMB_3: return HEX_MOCAP_JOINT_THUMB_IP;
    case HEX_MOCAP_DISP_THUMB_4: return -1;
    case HEX_MOCAP_DISP_INDEX_1: return HEX_MOCAP_JOINT_INDEX_MCP;
    case HEX_MOCAP_DISP_INDEX_2: return HEX_MOCAP_JOINT_INDEX_PIP;
    case HEX_MOCAP_DISP_INDEX_3: return HEX_MOCAP_JOINT_INDEX_DIP;
    case HEX_MOCAP_DISP_INDEX_4: return -1;
    case HEX_MOCAP_DISP_MIDDLE_1: return HEX_MOCAP_JOINT_MIDDLE_MCP;
    case HEX_MOCAP_DISP_MIDDLE_2: return HEX_MOCAP_JOINT_MIDDLE_PIP;
    case HEX_MOCAP_DISP_MIDDLE_3: return HEX_MOCAP_JOINT_MIDDLE_DIP;
    case HEX_MOCAP_DISP_MIDDLE_4: return -1;
    case HEX_MOCAP_DISP_RING_1: return HEX_MOCAP_JOINT_RING_MCP;
    case HEX_MOCAP_DISP_RING_2: return HEX_MOCAP_JOINT_RING_PIP;
    case HEX_MOCAP_DISP_RING_3: return HEX_MOCAP_JOINT_RING_DIP;
    case HEX_MOCAP_DISP_RING_4: return -1;
    case HEX_MOCAP_DISP_LITTLE_1: return HEX_MOCAP_JOINT_LITTLE_MCP;
    case HEX_MOCAP_DISP_LITTLE_2: return HEX_MOCAP_JOINT_LITTLE_PIP;
    case HEX_MOCAP_DISP_LITTLE_3: return HEX_MOCAP_JOINT_LITTLE_DIP;
    case HEX_MOCAP_DISP_LITTLE_4: return -1;
    default: return -1;
    }
}

/** Default DexterousHandNodeRecorder numeric id for display slot (left hand 1..21). */
static inline int hex_mocap_default_display_recorder_id_left(int display_slot)
{
    if (display_slot < 0 || display_slot >= HEX_MOCAP_DISP_JOINT_COUNT)
        return -1;
    return display_slot + 1;
}

/** Default Recorder numeric id for display slot (right hand 22..42). */
static inline int hex_mocap_default_display_recorder_id_right(int display_slot)
{
    if (display_slot < 0 || display_slot >= HEX_MOCAP_DISP_JOINT_COUNT)
        return -1;
    return display_slot + 22;
}

#ifdef __cplusplus
}
#endif
