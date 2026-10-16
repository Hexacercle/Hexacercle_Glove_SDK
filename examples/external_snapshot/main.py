"""统一快照示例：连接、诊断、R、骨骼、角度和带时间戳的流。Python 3.9+。"""
import argparse
import ctypes as C
import csv
import json
import os
import sys
import time
from contextlib import ExitStack
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from python_quickstart.main import Pose, Device, Model, RInfo, ANGLE_FIELDS, bind


class AbiInfo(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in (
        "struct_size", "api_version", "input_version", "pointer_size", "quat_size",
        "input_hand_size", "input_frame_size", "skeleton_size", "joint_angles_size",
        "calib_size", "input_timestamp_offset", "input_left_offset", "input_right_offset",
        "hand_root_offset", "hand_tip_offset", "skeleton_left_offset", "skeleton_right_offset",
        "skeleton_status_offset", "angles_left_offset", "angles_right_offset",
        "angles_status_offset", "real_joint_count", "display_joint_count")]


class Diagnostics(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("version", C.c_uint32)]
    _fields_ += [(name, C.c_uint64) for name in (
        "bytes", "frames", "checksum_mismatches", "framing_errors")]
    _fields_ += [("rotations", C.c_uint64 * 4)]


class Frame(C.Structure):
    # 字段顺序和默认对齐必须与 hex_external_api.h 完全一致。
    _fields_ = [("struct_size", C.c_uint32), ("version", C.c_uint32),
                ("update_index", C.c_uint64), ("host_timestamp_ms", C.c_int64),
                ("mocap_status", C.c_uint32), ("tracked_hand_mask", C.c_uint32),
                ("fresh_hand_mask", C.c_uint32), ("angle_valid_mask", C.c_uint32),
                ("pose", Pose * 2), ("flexion_deg", (C.c_float * 15) * 2)]


class ErrorInfo(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("code", C.c_uint32),
                ("message_utf8", C.c_char * 128)]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("port", help="COM3 或 /dev/ttyUSB0")
    p.add_argument("--lib", default=os.environ.get("HEX_GLOVE_LIBRARY",
                   "HexaCercleGlove.dll" if os.name == "nt" else "libHexaCercleGlove.so"))
    p.add_argument("--profile", help="已保存的 .hexprofile")
    p.add_argument("--r", action="store_true", help="加载 profile 后仍执行普通 R")
    p.add_argument("--csv", help="输出逐帧角度 CSV")
    p.add_argument("--jsonl", help="输出 JSON Lines；- 表示 stdout 实时流")
    p.add_argument("--ticks", type=int, default=300)
    a = p.parse_args()
    lib = C.CDLL(a.lib)
    H = C.c_void_p
    glove_create = bind(lib, "hex_glove_create", H)
    glove_abort = bind(lib, "hex_glove_abort", None, H)
    glove_destroy = bind(lib, "hex_glove_destroy", None, H)
    glove_diag = bind(lib, "hex_glove_get_receive_diagnostics", C.c_int, H, C.POINTER(Diagnostics))
    get_device_id = bind(lib, "hex_glove_get_device_id", C.c_int, H, C.c_void_p, C.c_uint32)
    request_device_id = bind(lib, "hex_glove_request_device_id", C.c_int, H)
    glove_device = bind(lib, "hex_glove_get_device_snapshot", C.c_int, H, C.POINTER(Device))
    glove_route = bind(lib, "hex_glove_send_get_route", None, H)
    mocap_create = bind(lib, "hex_mocap_create", H, H)
    mocap_destroy = bind(lib, "hex_mocap_destroy", None, H)
    mocap_status = bind(lib, "hex_mocap_get_status", C.c_uint32, H)
    capabilities = bind(lib, "hex_mocap_get_capabilities", C.c_uint32)
    abi_info = bind(lib, "hex_mocap_get_abi_info", C.c_int, C.POINTER(AbiInfo))
    model_init = bind(lib, "hex_mocap_skeleton_definition_init", C.c_int, C.POINTER(Model), C.c_int)
    model_set = bind(lib, "hex_mocap_set_skeleton_definition", C.c_int, H, C.c_int, C.POINTER(Model))
    r_start = bind(lib, "hex_mocap_start_r_capture", C.c_int, H, C.c_int, C.c_float)
    r_info = bind(lib, "hex_mocap_get_r_capture_info", C.c_int, H, C.POINTER(RInfo))
    external_create = bind(lib, "hex_external_create", H, H, H)
    external_destroy = bind(lib, "hex_external_destroy", None, H)
    external_version = bind(lib, "hex_external_get_api_version", C.c_uint32)
    external_profile = bind(lib, "hex_external_profile_load", C.c_int, H, C.c_char_p)
    external_start = bind(lib, "hex_external_start", C.c_int, H, C.c_char_p)
    external_update = bind(lib, "hex_external_update", C.c_int, H)
    external_frame = bind(lib, "hex_external_get_frame", C.c_int, H, C.POINTER(Frame))
    external_zero = bind(lib, "hex_external_calibrate_angles", C.c_int, H, C.c_uint32)
    external_error = bind(lib, "hex_external_get_last_error", C.c_int, H, C.POINTER(ErrorInfo))

    def fail(external, operation):
        info = ErrorInfo()
        info.struct_size = C.sizeof(info)
        if external_error(external, C.byref(info)):
            raise RuntimeError(f"{operation}: SDK {info.code}: {info.message_utf8.decode('utf-8')}")
        raise RuntimeError(operation)

    # 启动时核对 ABI，防止 ctypes 结构与所加载的库不匹配。
    abi = AbiInfo()
    abi.struct_size = C.sizeof(abi)
    if not abi_info(C.byref(abi)) or abi.pointer_size != C.sizeof(H) or abi.display_joint_count != 21:
        raise RuntimeError("不兼容的 SDK ABI")
    print(f"ABI=0x{abi.api_version:08x} external=0x{external_version():08x} "
          f"capabilities=0x{capabilities():08x}", file=sys.stderr)

    glove = glove_create()
    if not glove:
        raise RuntimeError("hex_glove_create failed")
    mocap = None
    external = None
    try:
        mocap = mocap_create(glove)
        external = external_create(glove, mocap) if mocap else None
        if not external:
            raise RuntimeError("cannot create external session")
        if a.profile:
            if not external_profile(external, a.profile.encode("utf-8")):
                fail(external, "profile load")
        else:
            # 没有 profile 时，只创建演示骨架；不能代表实际校准效果。
            for hand in (0, 1):
                model = Model()
                model.struct_size = C.sizeof(model)
                if model_init(C.byref(model), hand):
                    model_set(mocap, hand, C.byref(model))
        if not external_start(external, a.port.encode("utf-8")):
            fail(external, "serial start")
        # SDK 启动时先请求接收器 ID；信道查询等 ID 回复或超时。
        route_requested = False
        calibrated = 0
        r_started = False
        shown_device_id = False
        with ExitStack() as files:
            csv_file = files.enter_context(open(a.csv, "w", newline="", encoding="utf-8")) if a.csv else None
            writer = csv.writer(csv_file) if csv_file else None
            if writer:
                writer.writerow(("timestamp_ms", "utc_timestamp_ms", "update_index",
                                 "frame_sequence", "hand") + ANGLE_FIELDS)
            json_file = (sys.stdout if a.jsonl == "-" else
                         files.enter_context(open(a.jsonl, "w", encoding="utf-8")) if a.jsonl else None)
            for tick in range(a.ticks):
                if not shown_device_id:
                    receiver_id = C.create_string_buffer(7)
                    if get_device_id(glove, receiver_id, C.sizeof(receiver_id)):
                        print("receiver_device_id=" + receiver_id.value.decode("ascii"), file=sys.stderr)
                        shown_device_id = True
                if not shown_device_id and tick in (5, 30):
                    request_device_id(glove)
                if not route_requested and (shown_device_id or tick >= 20):
                    glove_route(glove)
                    route_requested = True
                # 一次更新同时缓存左右手 21 节点和本轮角度。
                if not external_update(external):
                    fail(external, "update")
                frame = Frame()
                frame.struct_size = C.sizeof(frame)
                if not external_frame(external, C.byref(frame)):
                    fail(external, "get frame")
                device = Device()
                glove_device(glove, C.byref(device))
                online = int(bool(device.left_enabled)) | (int(bool(device.right_enabled)) << 1)
                if (not a.profile or a.r) and not r_started and online:
                    r_started = bool(r_start(mocap, online, C.c_float(1.0)))
                r = RInfo()
                r.struct_size = C.sizeof(r)
                r_info(mocap, C.byref(r))
                need_zero = frame.tracked_hand_mask & frame.fresh_hand_mask & ~calibrated
                if ((a.profile and not a.r) or r.state == 2) and need_zero:
                    # R 是传感器零位；这里再做关节角的伸直中立零位。
                    if external_zero(external, need_zero):
                        calibrated |= need_zero
                    else:
                        fail(external, "angle calibration")
                if tick % 10 == 0:
                    diag = Diagnostics()
                    diag.struct_size = C.sizeof(diag)
                    glove_diag(glove, C.byref(diag))
                    print(f"status=0x{mocap_status(mocap):x} online={online} "
                          f"fresh={frame.fresh_hand_mask} tracked={frame.tracked_hand_mask} "
                          f"rx_frames={diag.frames} checksum={diag.checksum_mismatches}", file=sys.stderr)
                utc_ms = time.time_ns() // 1_000_000
                for hand in (0, 1):
                    bit = 1 << hand
                    if not (frame.fresh_hand_mask & frame.angle_valid_mask & bit):
                        continue  # 等待新帧或角度校准，避免重复导出旧值。
                    pose = frame.pose[hand]  # 本手 21 节点：world_position、local_rotation 等。
                    row = {"timestamp_ms": pose.timestamp_ms, "utc_timestamp_ms": utc_ms,
                           "update_index": frame.update_index,
                           "frame_sequence": pose.frame_sequence,
                           "hand": "left" if hand == 0 else "right"}
                    row.update(zip(ANGLE_FIELDS, (float(v) for v in frame.flexion_deg[hand])))
                    if writer:
                        writer.writerow(row.values())
                        csv_file.flush()
                    if json_file:
                        json_file.write(json.dumps(row, ensure_ascii=False) + "\n")
                        json_file.flush()
                time.sleep(0.1)
    finally:
        if external:
            external_destroy(external)
        if mocap:
            mocap_destroy(mocap)
        glove_abort(glove)
        glove_destroy(glove)


if __name__ == "__main__":
    main()
