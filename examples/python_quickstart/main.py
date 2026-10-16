"""手套连接、21 节点骨骼和关节角示例；Python 3.9+，仅使用标准库。"""
import argparse
import ctypes as C
import csv
import json
import os
import sys
import time
from contextlib import ExitStack


# 以下 ctypes.Structure 必须与 SDK 头文件的字段顺序、类型及默认对齐完全一致。
class Quat(C.Structure):
    _fields_ = [(n, C.c_float) for n in ("x", "y", "z", "w")]


class Vec3(C.Structure):
    _fields_ = [(n, C.c_float) for n in ("x", "y", "z")]


class Device(C.Structure):
    _fields_ = [(n, C.c_int32) for n in (
        "left_enabled", "right_enabled", "left_new_node", "right_new_node",
        "layout_left", "layout_right", "product_left", "product_right",
        "route", "route_received")]
    _fields_ += [("batt_left", C.c_double), ("batt_right", C.c_double),
                 ("hw_check_left", C.c_uint8 * 18), ("hw_check_right", C.c_uint8 * 18),
                 ("hw_check_left_valid", C.c_int32), ("hw_check_right_valid", C.c_int32)]


class Model(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("version", C.c_uint32),
                ("parent", C.c_int32 * 21), ("rest_local_position", Vec3 * 21),
                ("rest_local_rotation", Quat * 21), ("rest_local_scale", Vec3 * 21),
                ("root_position", Vec3), ("root_rotation", Quat), ("root_scale", Vec3)]


class Pose(C.Structure):
    """最终骨骼快照：21 个节点、跟踪有效位、单调时间戳与帧号。"""
    _fields_ = [("struct_size", C.c_uint32), ("version", C.c_uint32),
                ("timestamp_ms", C.c_int64), ("frame_sequence", C.c_uint32),
                ("valid_mask", C.c_uint32), ("tracked_mask", C.c_uint32),
                ("status_flags", C.c_uint32), ("world_position", Vec3 * 21),
                ("world_rotation", Quat * 21), ("local_position", Vec3 * 21),
                ("local_rotation", Quat * 21), ("local_scale", Vec3 * 21),
                ("world_scale", Vec3 * 21), ("world_matrix", (C.c_float * 16) * 21)]


class RInfo(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("state", C.c_int32),
                ("hand_mask", C.c_int32), ("progress", C.c_float),
                ("fresh_hand_mask", C.c_uint32)]


class AngleJoint(C.Structure):
    _fields_ = [(n, C.c_int32) for n in (
        "joint_type", "enabled", "enable_abduction", "clamp", "default_range")]
    _fields_ += [("flexion_axis", Vec3), ("abduction_axis", Vec3)]
    _fields_ += [(n, C.c_float) for n in (
        "flexion_min", "flexion_max", "abduction_min", "abduction_max")]


class HubConfig(C.Structure):
    """JointAngleHub 的 21 槽配置，左右手分别初始化一份。"""
    _fields_ = [("struct_size", C.c_uint32), ("version", C.c_uint32),
                ("joint", AngleJoint * 21), ("mcp_plane_normal_local", Vec3),
                ("spread_bone_direction_local", Vec3), ("spread_palm_normal_local", Vec3),
                ("mcp_absolute", C.c_int32), ("sequential_twist", C.c_int32),
                ("smooth_output", C.c_int32), ("smooth_speed", C.c_float),
                ("spread_mode", C.c_int32), ("spread_axis_mode", C.c_int32),
                ("spread_normal_source", C.c_int32), ("spread_anchor_offset", C.c_int32)]


class HubState(C.Structure):
    """JointAngleHub 零位与本帧 flexion/abduction 输出。"""
    _fields_ = [(n, C.c_uint32) for n in ("struct_size", "version", "calibrated", "valid_mask")]
    _fields_ += [("neutral_rotation", Quat * 21),
                 ("neutral_outgoing_world", Vec3 * 21),
                 ("neutral_outgoing_palm", Vec3 * 21)]
    _fields_ += [(n, C.c_float * 21) for n in (
        "neutral_flexion", "flexion", "abduction", "abduction_geometric")]
    _fields_ += [("spread", C.c_float * 5),
                 ("thumb_elevation", C.c_float), ("thumb_vs_index", C.c_float),
                 ("thumb_nail_to_palm", C.c_float), ("spread_valid", C.c_uint32),
                 ("thumb_aux_valid", C.c_uint32)]


class PreciseAngles(C.Structure):
    """mocap 内置精确角快照；finger[5][4] 不含单独的拇指 CMC。"""
    _fields_ = [(n, C.c_uint32) for n in ("struct_size", "version", "valid_mask", "calibrated")]
    _fields_ += [("timestamp_ms", C.c_int64), ("frame_sequence", C.c_uint32),
                 ("flags", C.c_uint32), ("finger", (C.c_float * 4) * 5),
                 ("ui_finger", (C.c_float * 4) * 5), ("palm_twist_degrees", Vec3)]
    _fields_ += [(n, C.c_float) for n in (
        "thumb_twist_degrees", "thumb_plane_degrees", "thumb_hardware_smoothed_degrees")]
    _fields_ += [("thumb_hardware_255", C.c_int32),
                 ("thumb_flexion_readout_degrees", C.c_float),
                 ("thumb_to_tip_mm", C.c_float * 4),
                 ("wrist_speed_deg_sec", C.c_float), ("wrist_smooth_blend", C.c_float),
                 ("finger_speed_deg_sec", C.c_float * 5)]
    _fields_ += [(n, C.c_uint32) for n in (
        "flex_hold_mask", "spread_hold_mask", "static_hold_mask", "thumb_plane_valid")]


ANGLE_FIELDS = (
    # CSV/JSON 的字段名与下方 Hub 槽位一一对应，角度单位为度。
    "thumb_cmc", "thumb_mcp", "thumb_ip",
    "index_mcp", "index_pip", "index_dip",
    "middle_mcp", "middle_pip", "middle_dip",
    "ring_mcp", "ring_pip", "ring_dip",
    "little_mcp", "little_pip", "little_dip")
ANGLE_SLOTS = (1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 17, 18, 19)


def bind(lib, name, result, *args):
    """给 C 函数声明返回值和参数类型，避免 ctypes 按默认 int 误解指针。"""
    fn = getattr(lib, name)
    fn.restype, fn.argtypes = result, list(args)
    return fn


def main():
    """按连接、加载校准、R、逐帧读取、导出、断开的顺序运行示例。"""
    p = argparse.ArgumentParser()
    p.add_argument("port", help="COM3 or /dev/ttyUSB0")
    p.add_argument("--lib", default=os.environ.get("HEX_GLOVE_LIBRARY",
        "HexaCercleGlove.dll" if os.name == "nt" else "libHexaCercleGlove.so"))
    p.add_argument("--profile", help="load an existing .hexprofile before connecting")
    p.add_argument("--r", action="store_true", help="run ordinary R after loading a profile")
    p.add_argument("--csv", help="write timestamped joint angles to CSV")
    p.add_argument("--jsonl", help="write timestamped JSON Lines; '-' streams to stdout")
    args = p.parse_args()
    lib = C.CDLL(args.lib)
    H = C.c_void_p
    create = bind(lib, "hex_glove_create", H)
    # hex_glove_* 管理串口与设备状态；hex_mocap_* 管理骨骼求解与 R。
    start = bind(lib, "hex_glove_start", C.c_int, H, C.c_char_p)
    get_device_id = bind(lib, "hex_glove_get_device_id", C.c_int, H, C.c_void_p, C.c_uint32)
    abort = bind(lib, "hex_glove_abort", None, H)
    destroy = bind(lib, "hex_glove_destroy", None, H)
    snapshot = bind(lib, "hex_glove_get_device_snapshot", C.c_int, H, C.POINTER(Device))
    mocap_create = bind(lib, "hex_mocap_create", H, H)
    mocap_destroy = bind(lib, "hex_mocap_destroy", None, H)
    profile_load = bind(lib, "hex_mocap_profile_load", C.c_int, H, C.c_char_p)
    update = bind(lib, "hex_mocap_update", C.c_int, H)
    model_init = bind(lib, "hex_mocap_skeleton_definition_init", C.c_int, C.POINTER(Model), C.c_int)
    model_set = bind(lib, "hex_mocap_set_skeleton_definition", C.c_int, H, C.c_int, C.POINTER(Model))
    r_capture = bind(lib, "hex_mocap_start_r_capture", C.c_int, H, C.c_int, C.c_float)
    get_r_info = bind(lib, "hex_mocap_get_r_capture_info", C.c_int, H, C.POINTER(RInfo))
    get_pose = bind(lib, "hex_mocap_get_display_pose", C.c_int, H, C.c_int, C.POINTER(Pose))
    hub_config_init = bind(lib, "hex_angle_hub_config_init", C.c_int, C.POINTER(HubConfig))
    # Hub 输出含 CMC 的 21 槽角度；Precise 输出 finger[5][4]，两套算法分开显示。
    hub_state_init = bind(lib, "hex_angle_hub_state_init", C.c_int, C.POINTER(HubState))
    hub_calibrate = bind(lib, "hex_angle_hub_calibrate", C.c_int,
                         C.POINTER(HubConfig), C.POINTER(HubState), C.POINTER(Pose), C.c_void_p)
    hub_update = bind(lib, "hex_angle_hub_update", C.c_int,
                      C.POINTER(HubConfig), C.POINTER(HubState), C.POINTER(Pose),
                      C.c_float, C.c_void_p, C.c_void_p)
    calibrate_precise = bind(lib, "hex_mocap_calibrate_angles", C.c_int, H, C.c_int)
    get_precise = bind(lib, "hex_mocap_get_precise_angles", C.c_int,
                       H, C.c_int, C.POINTER(PreciseAngles))
    configs, states = (HubConfig(), HubConfig()), (HubState(), HubState())
    for config, state in zip(configs, states):
        # 所有带 struct_size 的 C 结构必须先填自身大小，再交给 SDK 初始化。
        config.struct_size, state.struct_size = C.sizeof(config), C.sizeof(state)
        if not hub_config_init(C.byref(config)) or not hub_state_init(C.byref(state)):
            raise RuntimeError("Cannot initialize joint angle hub")
    calibrated = [False, False]
    last_frame = [(-1, -1), (-1, -1)]
    log = lambda *items: print(*items, file=sys.stderr)
    glove = create()
    if not glove:
        raise RuntimeError("hex_glove_create failed")
    mocap = None
    with ExitStack() as files:
      csv_file = files.enter_context(open(args.csv, "w", newline="", encoding="utf-8")) if args.csv else None
      writer = csv.writer(csv_file) if csv_file else None
      if writer:
          # CSV 第一行写固定字段名；后面每个有效手的新帧对应一行。
          writer.writerow(("timestamp_ms", "utc_timestamp_ms", "frame_sequence", "hand") + ANGLE_FIELDS)
      json_file = (sys.stdout if args.jsonl == "-" else
                   files.enter_context(open(args.jsonl, "w", encoding="utf-8")) if args.jsonl else None)
      log("Keep fingers straight and neutral until angle zero is captured after R.")
      try:
        mocap = mocap_create(glove)
        if not mocap:
            raise RuntimeError("hex_mocap_create failed")
        if args.profile and not profile_load(mocap, args.profile.encode("utf-8")):
            # profile 保存骨架与校准，先加载再打开串口/开始逐帧求解。
            raise RuntimeError("Cannot load profile: " + args.profile)
        if not start(glove, args.port.encode("utf-8")):
            raise RuntimeError("Cannot open " + args.port)
        for hand in (range(2) if not args.profile else ()):
            # 没有 profile 时用合成骨架验证数据链路；不能代表真实手型。
            model = Model()
            model.struct_size = C.sizeof(Model)
            if not model_init(C.byref(model), hand) or not model_set(mocap, hand, C.byref(model)):
                raise RuntimeError("Cannot install diagnostic skeleton")
        r_started = False
        shown_device_id = False
        while True:
            # 串口启动时 SDK 自动发送读 ID 指令；回复到达前返回 0。
            if not shown_device_id:
                receiver_id = C.create_string_buffer(7)
                if get_device_id(glove, receiver_id, C.sizeof(receiver_id)):
                    log("receiver_device_id=" + receiver_id.value.decode("ascii"))
                    shown_device_id = True
            device = Device()
            snapshot(glove, C.byref(device))
            update(mocap)
            # 每次 update 推进传感器帧、R 采样与最终骨骼状态。
            mask = int(bool(device.left_enabled)) | (2 if device.right_enabled else 0)
            if mask and not r_started and (not args.profile or args.r):
                r_started = bool(r_capture(mocap, mask, C.c_float(1.0)))
            r_info = RInfo()
            r_info.struct_size = C.sizeof(RInfo)
            get_r_info(mocap, C.byref(r_info))
            angle_ready = (bool(args.profile) and not args.r) or r_info.state == 2
            # 传感器 R 与关节角零位不同；重新 R 完成后才建立几何角零位。
            log(f"route={device.route} L=M{device.layout_left}/{device.batt_left:.0f}% "
                f"R=M{device.layout_right}/{device.batt_right:.0f}%")
            for hand in range(2):
                pose = Pose()
                pose.struct_size = C.sizeof(Pose)
                if get_pose(mocap, hand, C.byref(pose)):
                    v = pose.world_position[0]
                    log(f"  skeleton valid={pose.valid_mask:08x} tracked={pose.tracked_mask:08x} "
                        f"palm=({v.x:.3f},{v.y:.3f},{v.z:.3f}) m")
                    required = all(pose.tracked_mask & (1 << slot) for slot in ANGLE_SLOTS)
                    # 只接受完整的实时跟踪；静态模型预览和缺关节帧不导出。
                    if not angle_ready or not required:
                        continue
                    if not calibrated[hand]:
                        calibrated[hand] = bool(hub_calibrate(C.byref(configs[hand]),
                                                              C.byref(states[hand]), C.byref(pose), None))
                        if calibrated[hand]:
                            # 同一伸直姿态也校准内置精确角，但导出字段仍统一用 Hub。
                            calibrate_precise(mocap, 1 << hand)
                            last_frame[hand] = (pose.timestamp_ms, pose.frame_sequence)
                            log(f"  {'right' if hand else 'left'} angle zero captured")
                        continue
                    previous_ms, previous_seq = last_frame[hand]
                    if pose.timestamp_ms <= previous_ms or pose.frame_sequence == previous_seq:
                        # 排除重复/倒退帧，避免给旧角度配新 UTC 时间。
                        continue
                    dt = (pose.timestamp_ms - previous_ms) / 1000.0
                    if dt > 10 or not hub_update(C.byref(configs[hand]), C.byref(states[hand]),
                                                 C.byref(pose), C.c_float(dt), None, None):
                        continue
                    last_frame[hand] = (pose.timestamp_ms, pose.frame_sequence)
                    if not all(states[hand].valid_mask & (1 << slot) for slot in ANGLE_SLOTS):
                        continue
                    values = [float(states[hand].flexion[slot]) for slot in ANGLE_SLOTS]
                    utc_ms = time.time_ns() // 1_000_000
                    row = (pose.timestamp_ms, utc_ms, pose.frame_sequence,
                           "right" if hand else "left", *values)
                    if writer:
                        # SDK 时间是单调毫秒；UTC Unix 毫秒由 Python 宿主时钟提供。
                        writer.writerow(row)
                        csv_file.flush()
                    if json_file:
                        # 一帧一行 JSON，--jsonl - 可通过标准输出管道实时消费。
                        record = dict(zip(("timestamp_ms", "utc_timestamp_ms", "frame_sequence", "hand")
                                          + ANGLE_FIELDS, row))
                        json_file.write(json.dumps(record, ensure_ascii=False) + "\n")
                        json_file.flush()
                    log("  angles(deg): " + " ".join(f"{n}={x:.1f}" for n, x in zip(ANGLE_FIELDS, values)))
                    precise = PreciseAngles()
                    precise.struct_size = C.sizeof(PreciseAngles)
                    if get_precise(mocap, hand, C.byref(precise)) and precise.calibrated:
                        # 精确角快照自带时间戳；CMC 必须从 Hub 读取。
                        log(f"  precise t={precise.timestamp_ms}ms thumb_mcp={precise.finger[0][0]:.1f} "
                            f"index_mcp={precise.finger[1][0]:.1f}° (CMC is in Hub)")
            time.sleep(0.1)
      except KeyboardInterrupt:
          pass
      finally:
        # 释放顺序：mocap 会话 → 关闭串口 → glove 会话。
        if mocap:
            mocap_destroy(mocap)
        abort(glove)
        destroy(glove)


if __name__ == "__main__":
    main()
