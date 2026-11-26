#pragma once

#include "hex_glove/hex_mocap_api.h"

class QWidget;

#if defined(HEX_QT_FBX_BUILD)
#define HEX_QT_FBX_API __declspec(dllexport)
#else
#define HEX_QT_FBX_API __declspec(dllimport)
#endif

extern "C" {
// Qt 6.5.3 MSVC x64 Release. Create and update on the Qt GUI thread.
// The parent owns the widget. A widget without a parent can be deleted by its owner.
HEX_QT_FBX_API QWidget* hex_qt_fbx_create(QWidget* parent);
// hand: 0 = left, 1 = right. Set pose->struct_size. Null pose restores standby.
// Supply the final pose for the bundled Unity hand model, with world matrices in metres.
HEX_QT_FBX_API int hex_qt_fbx_set_pose(QWidget* view, int hand, const HexMocapDisplayPose* pose);
// Optional fixed view axes in SDK world coordinates. Both axes are required.
HEX_QT_FBX_API int hex_qt_fbx_set_pose_with_basis(QWidget* view, int hand,
    const HexMocapDisplayPose* pose, const HexMocapVec3* up, const HexMocapVec3* right);
// Read the built-in standby pose. Set out->struct_size before calling.
HEX_QT_FBX_API int hex_qt_fbx_get_idle_pose(int hand, HexMocapDisplayPose* out);

// Configure the bundled Unity rig on a fresh SDK mocap handle, before calibration.
// This installs the default M11 mapping. A matching profile can be loaded instead.
HEX_QT_FBX_API int hex_qt_fbx_configure_default_model(HexMocapHandle mocap);
// Returns the observed layout (7 or 11), or 0 before it is known.
HEX_QT_FBX_API int hex_qt_fbx_get_input_layout(HexMocapHandle mocap, int hand);
// Select the source rig before calibration, or after a device layout change.
HEX_QT_FBX_API int hex_qt_fbx_set_input_layout(HexMocapHandle mocap, int hand, int layout);
// Start the two-second remount calibration. Mask 1 = left, 2 = right, 3 = both.
HEX_QT_FBX_API int hex_qt_fbx_start_quick_r(HexMocapHandle mocap, int hand_mask);
}
