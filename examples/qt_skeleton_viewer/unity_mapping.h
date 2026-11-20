#pragma once

#include "hex_glove/hex_mocap_api.h"

bool installUnityMapping(HexMocapHandle mocap);
bool applyUnityInputLayout(HexMocapHandle mocap, int hand, int layout);
void applyUnityIdlePreview(int hand, HexMocapDisplayPose& pose);
