#!/usr/bin/env python3
"""XGF SDK -> 21-node MuJoCo viewer, without Unity or ROS."""

import argparse
import ctypes as C
import json
import os
from pathlib import Path
import time


NODES = 21
ALL_NODES = (1 << NODES) - 1
DEFAULT_PARENT = (-1, 0, 1, 2, 3, 0, 5, 6, 7, 0, 9, 10, 11, 0, 13, 14, 15, 0, 17, 18, 19)


class Vec3(C.Structure):
    """与 SDK C ABI 一致的三维向量，用于骨骼位置。"""
    _fields_ = [(name, C.c_float) for name in ("x", "y", "z")]


class Quat(C.Structure):
    """SDK 四元数，字段顺序为 x、y、z、w。"""
    _fields_ = [(name, C.c_float) for name in ("x", "y", "z", "w")]


class Skeleton(C.Structure):
    """上传给 SDK 的 21 节点骨架定义；struct_size 必须先填写。"""
    _fields_ = [
        ("struct_size", C.c_uint32), ("version", C.c_uint32),
        ("parent", C.c_int32 * NODES),
        ("rest_local_position", Vec3 * NODES),
        ("rest_local_rotation", Quat * NODES),
        ("rest_local_scale", Vec3 * NODES),
        ("root_position", Vec3), ("root_rotation", Quat), ("root_scale", Vec3),
    ]


class Pose(C.Structure):
    """最终显示骨骼：节点位置/旋转、跟踪位、时间戳和帧号。"""
    _fields_ = [
        ("struct_size", C.c_uint32), ("version", C.c_uint32),
        ("timestamp_ms", C.c_int64), ("frame_sequence", C.c_uint32),
        ("valid_mask", C.c_uint32), ("tracked_mask", C.c_uint32),
        ("status_flags", C.c_uint32), ("world_position", Vec3 * NODES),
        ("world_rotation", Quat * NODES), ("local_position", Vec3 * NODES),
        ("local_rotation", Quat * NODES), ("local_scale", Vec3 * NODES),
        ("world_scale", Vec3 * NODES),
        ("world_matrix", (C.c_float * 16) * NODES),
    ]


class Device(C.Structure):
    """连接状态、电量、布局和信道等设备基础信息。"""
    _fields_ = [(name, C.c_int32) for name in (
        "left_enabled", "right_enabled", "left_new_node", "right_new_node",
        "layout_left", "layout_right", "product_left", "product_right",
        "route", "route_received")]
    _fields_ += [
        ("batt_left", C.c_double), ("batt_right", C.c_double),
        ("hw_check_left", C.c_uint8 * 18), ("hw_check_right", C.c_uint8 * 18),
        ("hw_check_left_valid", C.c_int32), ("hw_check_right_valid", C.c_int32),
    ]


class RInfo(C.Structure):
    """普通 R 校准的状态、目标手与采样进度。"""
    _fields_ = [
        ("struct_size", C.c_uint32), ("state", C.c_int32),
        ("hand_mask", C.c_int32), ("progress", C.c_float),
        ("fresh_hand_mask", C.c_uint32),
    ]


def bind(lib, name, result, *args):
    """声明 ctypes 函数签名，确保句柄和结构指针按 C ABI 传递。"""
    fn = getattr(lib, name)
    fn.restype, fn.argtypes = result, list(args)
    return fn


def connect_api(path):
    """加载当前平台的 SDK 动态库，并绑定示例实际使用的公开 API。"""
    dll_dir = os.add_dll_directory(str(path.parent)) if os.name == "nt" else None
    lib = C.CDLL(str(path))
    handle = C.c_void_p
    api = {
        # create/start/abort/destroy 管理串口会话；mocap_* 管理骨骼会话。
        "create": bind(lib, "hex_glove_create", handle),
        "start": bind(lib, "hex_glove_start", C.c_int, handle, C.c_char_p),
        "abort": bind(lib, "hex_glove_abort", None, handle),
        "destroy": bind(lib, "hex_glove_destroy", None, handle),
        "device": bind(lib, "hex_glove_get_device_snapshot", C.c_int, handle, C.POINTER(Device)),
        "device_id": bind(lib, "hex_glove_get_device_id", C.c_int, handle, C.c_void_p, C.c_uint32),
        "mocap_create": bind(lib, "hex_mocap_create", handle, handle),
        "mocap_destroy": bind(lib, "hex_mocap_destroy", None, handle),
        "model_init": bind(lib, "hex_mocap_skeleton_definition_init", C.c_int, C.POINTER(Skeleton), C.c_int),
        "model_set": bind(lib, "hex_mocap_set_skeleton_definition", C.c_int, handle, C.c_int, C.POINTER(Skeleton)),
        "profile_load": bind(lib, "hex_mocap_profile_load", C.c_int, handle, C.c_char_p),
        "update": bind(lib, "hex_mocap_update", C.c_int, handle),
        "r_start": bind(lib, "hex_mocap_start_r_capture", C.c_int, handle, C.c_int, C.c_float),
        "r_info": bind(lib, "hex_mocap_get_r_capture_info", C.c_int, handle, C.POINTER(RInfo)),
        "pose": bind(lib, "hex_mocap_get_display_pose", C.c_int, handle, C.c_int, C.POINTER(Pose)),
    }
    return api, dll_dir


def configure(api, mocap, hands, skeleton_path, profile_path):
    """为选定手建立骨架父链，并恢复可选的 .hexprofile 校准文件。"""
    config = json.loads(skeleton_path.read_text(encoding="utf-8")) if skeleton_path else None
    parents = {}
    for hand in hands:
        # 无 JSON 时用 SDK 合成骨架；传入 JSON 时覆盖其节点 rest 变换。
        side = "left" if hand == 0 else "right"
        model = Skeleton()
        model.struct_size = C.sizeof(model)
        if not api["model_init"](C.byref(model), hand):
            raise RuntimeError(f"Cannot initialize {side} skeleton")
        if config:
            source = config["hands"][side]
            for field in ("parent", "rest_local_position", "rest_local_rotation", "rest_local_scale"):
                if len(source[field]) != NODES:
                    raise ValueError(f"{side}.{field} must have 21 entries")
                target = getattr(model, field)
                for index, value in enumerate(source[field]):
                    target[index] = int(value) if field == "parent" else target._type_(*value)
            for field, cls in (("root_position", Vec3), ("root_rotation", Quat), ("root_scale", Vec3)):
                setattr(model, field, cls(*source[field]))
        parents[hand] = tuple(model.parent) if config else DEFAULT_PARENT
        if not api["model_set"](mocap, hand, C.byref(model)):
            raise RuntimeError(f"Cannot install {side} skeleton")
    # A saved profile is loaded last: it may restore the calibrated model and R state.
    if profile_path and not api["profile_load"](mocap, str(profile_path).encode("utf-8")):
        raise RuntimeError(f"Cannot load SDK profile: {profile_path}")
    return parents


def palm_local_points(pose, np):
    """把 SDK 世界节点变换到手掌局部坐标，供 MuJoCo 固定视角显示。"""
    # SDK/Unity local +Z follows the fingers. MuJoCo uses +Z as up here.
    # TipSync negates coordinates before its robot retarget transform; this
    # direct skeleton viewer has no such later transform.
    if pose.valid_mask != ALL_NODES:
        return None
    points = np.array([(p.x, p.y, p.z) for p in pose.world_position], dtype=np.float64)
    palm = np.array(pose.world_matrix[0], dtype=np.float64).reshape(4, 4).T
    if not np.isfinite(points).all() or not np.isfinite(palm).all():
        return None
    try:
        local = np.linalg.solve(palm[:3, :3], (points - palm[:3, 3]).T).T
    except np.linalg.LinAlgError:
        return None
    if not np.isfinite(local).all():
        return None
    local[0] = 0
    return local


def draw(viewer, latest, parents, mujoco, np):
    """用球体表示关节、胶囊表示骨段；断流/未跟踪时将手显示成灰色。"""
    with viewer.lock():
        scene = viewer.user_scn
        scene.ngeom = 0
        for hand, (points, tracked_mask) in latest.items():
            offset = np.array((-.14 if hand == 0 else .14, 0., 0.))
            color = np.array((.22, .55, 1., 1.) if hand == 0 else (1., .45, .22, 1.), dtype=np.float32)
            if tracked_mask != ALL_NODES:
                color = np.array((.42, .42, .42, .65), dtype=np.float32)
            points = points + offset
            for index, point in enumerate(points):
                if scene.ngeom >= scene.maxgeom:
                    return
                mujoco.mjv_initGeom(scene.geoms[scene.ngeom], mujoco.mjtGeom.mjGEOM_SPHERE,
                                   np.array((.008 if index == 0 else .005, 0., 0.)),
                                   point, np.eye(3).ravel(), color)
                scene.ngeom += 1
                parent = parents[hand][index]
                if parent < 0 or scene.ngeom >= scene.maxgeom:
                    continue
                mujoco.mjv_initGeom(scene.geoms[scene.ngeom], mujoco.mjtGeom.mjGEOM_CAPSULE,
                                   np.zeros(3), np.zeros(3), np.zeros(9), color)
                mujoco.mjv_connector(scene.geoms[scene.ngeom], mujoco.mjtGeom.mjGEOM_CAPSULE,
                                    .003, points[parent], point)
                scene.ngeom += 1


def main():
    """解析参数，连接手套，逐帧更新 SDK，并把骨骼交给 MuJoCo 绘制。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lib", type=Path, help="SDK DLL or .so from library folder")
    parser.add_argument("--port", help="COM3 or /dev/ttyUSB0; omitted with --check or --check-gl")
    parser.add_argument("--hand", choices=("left", "right", "both"), default="right")
    folder = Path(__file__).resolve().parent
    default_skeleton = folder / "xgf_skeleton.json"
    default_profile = folder / "10030.hexprofile"
    parser.add_argument("--skeleton", type=Path,
                        default=default_skeleton if default_skeleton.is_file() else None,
                        help="21-node skeleton JSON; defaults to adjacent xgf_skeleton.json")
    parser.add_argument("--profile", type=Path,
                        default=default_profile if default_profile.is_file() else None,
                        help="SDK profile; defaults to adjacent 10030.hexprofile")
    parser.add_argument("--no-profile", action="store_true", help="ignore the adjacent default profile")
    parser.add_argument("--r", action="store_true", help="perform ordinary 2-second R even with a profile")
    parser.add_argument("--check", action="store_true", help="load SDK/configuration without serial port or viewer")
    parser.add_argument("--check-gl", action="store_true", help="test a GLFW OpenGL window without opening the glove")
    args = parser.parse_args()
    if args.check_gl:
        # 只检测 OpenGL/GLFW 环境，不打开手套串口。
        import glfw
        print(f"DISPLAY={os.environ.get('DISPLAY', '')} WAYLAND_DISPLAY={os.environ.get('WAYLAND_DISPLAY', '')}")
        if not glfw.init():
            raise SystemExit("GLFW initialization failed. Check this desktop session and run glxinfo -B.")
        window = None
        try:
            glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
            window = glfw.create_window(32, 32, "XGF OpenGL check", None, None)
            if window is None:
                raise SystemExit("OpenGL window creation failed. Try LIBGL_ALWAYS_SOFTWARE=true, then check glxinfo -B.")
            print("GLFW OpenGL context created successfully")
        finally:
            if window is not None:
                glfw.destroy_window(window)
            glfw.terminate()
        return
    if args.lib is None:
        parser.error("--lib is required unless --check-gl is used")
    if args.no_profile:
        args.profile = None
    if not args.check and not args.port:
        parser.error("--port is required unless --check is used")
    for path in (args.lib, args.skeleton, args.profile):
        if path and not path.is_file():
            parser.error(f"file does not exist: {path}")
    hands = (0, 1) if args.hand == "both" else (0,) if args.hand == "left" else (1,)
    # 双手 R 采用掩码 3，一次同时采样；单手分别为 1 或 2。
    hand_mask = sum(1 << hand for hand in hands)
    api, dll_dir = connect_api(args.lib.resolve())
    glove = mocap = None
    try:
        if not args.check:
            glove = api["create"]()
            if not glove:
                raise RuntimeError("hex_glove_create failed")
        mocap = api["mocap_create"](glove)
        if not mocap:
            raise RuntimeError("hex_mocap_create failed")
        parents = configure(api, mocap, hands, args.skeleton, args.profile)
        # --check 只验证 SDK 与配置是否可加载，不启动 MuJoCo 窗口。
        if args.check:
            print(f"SDK/configuration check passed; skeleton={args.skeleton}, profile={args.profile}; serial port was not opened")
            return
        import mujoco
        import mujoco.viewer
        import numpy as np
        model = mujoco.MjModel.from_xml_string(
            '<mujoco model="XGF 21-node viewer"><option timestep="0.01" gravity="0 0 0"/>'
            '<worldbody><light pos="0 0 1" dir="0 0 -1"/>'
            '<geom type="plane" size="1 1 .01" pos="0 0 -.18" rgba=".14 .14 .14 1"/>'
            '</worldbody></mujoco>')
        data = mujoco.MjData(model)
        r_key = [False]
        def on_key(key):
            """记录 R 键请求，实际 SDK 调用留给主循环串行执行。"""
            if key in (ord("R"), ord("r")):
                r_key[0] = True
        pending_r = not args.profile or args.r
        next_r_attempt = 0.
        last_r_state = None
        last_log = 0.
        with mujoco.viewer.launch_passive(model, data, key_callback=on_key) as viewer:
            if not api["start"](glove, args.port.encode("utf-8")):
                raise RuntimeError(f"Cannot open glove port: {args.port}")
            with viewer.lock():
                viewer.cam.lookat[:] = (0, 0, .04)
                viewer.cam.distance = .55
                viewer.cam.azimuth = 90
                viewer.cam.elevation = -15
            print(f"Connected. hands={args.hand}, profile={args.profile or '(none)'}. Press R for quick calibration; close window to exit.")
            device_id_shown = False
            while viewer.is_running():
                if not device_id_shown:
                    # SDK 在串口打开后自动请求接收器 ID；等待异步回复。
                    device_id = C.create_string_buffer(7)
                    if api["device_id"](glove, device_id, C.sizeof(device_id)):
                        print("receiver_device_id=" + device_id.value.decode("ascii"))
                        device_id_shown = True
                tick = time.monotonic()
                api["update"](mocap)
                # 每帧推进接收数据、R 进度和最终 21 节点骨骼求解。
                device = Device()
                api["device"](glove, C.byref(device))
                online = int(bool(device.left_enabled)) | (2 if device.right_enabled else 0)
                if r_key[0]:
                    pending_r = True
                    r_key[0] = False
                info = RInfo()
                info.struct_size = C.sizeof(info)
                has_r_info = bool(api["r_info"](mocap, C.byref(info)))
                if has_r_info and (info.state, info.hand_mask) != last_r_state:
                    label = {0: "idle", 1: "sampling", 2: "complete", 3: "failed", 4: "cancelled"}.get(info.state, info.state)
                    print(f"R state: {label}; hand_mask={info.hand_mask}; progress={info.progress:.0%}")
                    last_r_state = (info.state, info.hand_mask)
                # One selected hand uses mask 1/2; both hands use mask 3 in
                # a single capture. Never substitute two sequential R calls.
                if (pending_r and tick >= next_r_attempt and (online & hand_mask) == hand_mask
                        and (not has_r_info or info.state != 1)):
                    if api["r_start"](mocap, hand_mask, C.c_float(2.0)):
                        print(f"R started for {args.hand}: keep selected hand(s) flat for 2 seconds.")
                        pending_r = False
                    else:
                        print(f"R not ready for {args.hand}; waiting for fresh data from all selected hands.")
                    next_r_attempt = tick + 1.
                latest = {}
                for hand in hands:
                    pose = Pose()
                    pose.struct_size = C.sizeof(pose)
                    if api["pose"](mocap, hand, C.byref(pose)):
                        # tracked_mask 与 valid_mask 不同：只有前者表示实时跟踪。
                        points = palm_local_points(pose, np)
                        if points is not None:
                            latest[hand] = (points, int(pose.tracked_mask))
                draw(viewer, latest, parents, mujoco, np)
                viewer.sync()
                if tick - last_log >= 5.:
                    states = [f"{'left' if h == 0 else 'right'}:{'tracked' if h in latest and latest[h][1] == ALL_NODES else 'waiting'}" for h in hands]
                    print(f"online_mask={online} visible_models={len(latest)} {' '.join(states)}; pending_R={pending_r}")
                    last_log = tick
                time.sleep(max(0., 1. / 60. - (time.monotonic() - tick)))
    except KeyboardInterrupt:
        pass
    finally:
        # 按 mocap → 串口 abort → glove 的顺序释放 SDK 资源。
        if mocap:
            api["mocap_destroy"](mocap)
        if glove:
            api["abort"](glove)
            api["destroy"](glove)
        if dll_dir:
            dll_dir.close()


if __name__ == "__main__":
    main()
