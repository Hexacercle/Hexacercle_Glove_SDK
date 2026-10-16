#pragma once
#include "hex_glove/hex_external_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEX_PARTNER_API_VERSION 0x00030000u

enum HexPartnerPhase {
    HEX_PARTNER_STOPPED = 0, HEX_PARTNER_SEARCHING = 1,
    HEX_PARTNER_CONNECTING = 2, HEX_PARTNER_WAITING_DATA = 3,
    HEX_PARTNER_STREAMING = 4, HEX_PARTNER_DATA_INTERRUPTED = 5,
    HEX_PARTNER_RETRYING = 6, HEX_PARTNER_FAILED = 7,
    HEX_PARTNER_STREAM_PAUSED = 8
};
enum HexPartnerError {
    HEX_PARTNER_OK = 0, HEX_PARTNER_INVALID_ARGUMENT = 1,
    HEX_PARTNER_BAD_STRUCT_SIZE = 2, HEX_PARTNER_OPEN_FAILED = 3,
    HEX_PARTNER_RECEIVER_LOST = 4, HEX_PARTNER_DATA_TIMEOUT = 5,
    HEX_PARTNER_UPDATE_FAILED = 6, HEX_PARTNER_R_START_FAILED = 7,
    HEX_PARTNER_R_FAILED = 8, HEX_PARTNER_CALIBRATION_FAILED = 9,
    HEX_PARTNER_PROFILE_NOT_FOUND = 10, HEX_PARTNER_PROFILE_SAVE_FAILED = 11,
    HEX_PARTNER_PROFILE_LOAD_FAILED = 12, HEX_PARTNER_NO_PORT_BINDING = 13,
    HEX_PARTNER_NO_FRESH_FRAME = 14, HEX_PARTNER_ENCODE_FAILED = 15,
    HEX_PARTNER_PORT_NOT_FOUND = 16, HEX_PARTNER_PORT_ACCESS_DENIED = 17,
    HEX_PARTNER_PORT_CONFIG_FAILED = 18, HEX_PARTNER_START_COMMAND_FAILED = 19,
    HEX_PARTNER_STREAM_COMMAND_FAILED = 20
};
enum HexPartnerEventKind {
    HEX_PARTNER_EVENT_NONE = 0, HEX_PARTNER_EVENT_CONNECTED = 1,
    HEX_PARTNER_EVENT_STREAM_STARTED = 2, HEX_PARTNER_EVENT_DATA_INTERRUPTED = 3,
    HEX_PARTNER_EVENT_RETRYING = 4, HEX_PARTNER_EVENT_FAILED = 5,
    HEX_PARTNER_EVENT_DISCONNECTED = 6, HEX_PARTNER_EVENT_PROFILE_LOADED = 7,
    HEX_PARTNER_EVENT_R_COMPLETE = 8, HEX_PARTNER_EVENT_CALIBRATION_COMPLETE = 9,
    HEX_PARTNER_EVENT_CALIBRATION_FAILED = 10,
    HEX_PARTNER_EVENT_STREAM_PAUSED = 11,
    HEX_PARTNER_EVENT_STREAM_COMMAND_FAILED = 12
};
enum HexPartnerCalibrationMode { HEX_PARTNER_CALIBRATION_R = 0, HEX_PARTNER_CALIBRATION_FULL = 1 };
enum HexPartnerCalibrationResult {
    HEX_PARTNER_CALIBRATION_IDLE = 0, HEX_PARTNER_CALIBRATION_RUNNING = 1,
    HEX_PARTNER_CALIBRATION_SUCCEEDED = 2, HEX_PARTNER_CALIBRATION_CANCELLED = 3,
    HEX_PARTNER_CALIBRATION_ERROR = 4
};
enum HexPartnerOperationKind { HEX_PARTNER_OP_NONE = 0, HEX_PARTNER_OP_CONNECT = 1,
    HEX_PARTNER_OP_DISCONNECT = 2, HEX_PARTNER_OP_CALIBRATE = 3,
    HEX_PARTNER_OP_PROFILE_LOAD = 4, HEX_PARTNER_OP_PROFILE_SAVE = 5,
    HEX_PARTNER_OP_STREAM_START = 6, HEX_PARTNER_OP_STREAM_STOP = 7 };
enum HexPartnerOperationState { HEX_PARTNER_OP_IDLE = 0, HEX_PARTNER_OP_RUNNING = 1,
    HEX_PARTNER_OP_SUCCEEDED = 2, HEX_PARTNER_OP_FAILED = 3,
    HEX_PARTNER_OP_CANCELLED = 4 };

typedef struct HexPartnerConfig {
    uint32_t struct_size, version;
    int32_t auto_detect, auto_reconnect, probe_ms, reconnect_ms, data_timeout_ms;
    int32_t auto_load_profile, auto_save_profile;
    char preferred_port_utf8[256];
    char profile_dir_utf8[512];
} HexPartnerConfig;

typedef struct HexPartnerStatus {
    uint32_t struct_size, version, phase, receiver_phase;
    uint32_t online_hand_mask, fresh_hand_mask, streaming_hand_mask;
    uint32_t attempts, failures, last_error, event_sequence, last_event;
    int64_t timestamp_ms, last_fresh_ms[2];
    char port_utf8[256];
    char device_code[2][3]; /* Hardware's short code; NOT a unique serial number. */
    uint32_t profile_loaded;
    uint32_t operation_sequence, operation_kind, operation_state, operation_error;
    uint32_t port_error_stage, platform_error_code, stream_requested;
} HexPartnerStatus;

typedef struct HexPartnerTiming {
    uint32_t struct_size, version;
    float observed_hz[2]; /* Host-side fresh-frame rate, not hardware sample rate. */
    int64_t last_fresh_age_ms[2]; /* -1 before the first fresh frame. */
    int64_t hand_arrival_gap_ms; /* -1 until both hands have fresh data. */
} HexPartnerTiming;

typedef struct HexPartnerCalibrationStatus {
    uint32_t struct_size, version, mode, requested_hand_mask, result;
    uint32_t r_state, r_fresh_hand_mask, step[2];
    float r_progress, progress[2];
    uint32_t error;
    char prompt_utf8[2][256];
} HexPartnerCalibrationStatus;

typedef struct HexPartnerEvent {
    uint32_t struct_size, sequence, kind, error;
    int64_t timestamp_ms;
    uint32_t operation_sequence;
    char message_utf8[160];
} HexPartnerEvent;

typedef struct HexPartnerSession* HexPartnerHandle;
HEX_GLOVE_API uint32_t hex_partner_get_api_version(void);
HEX_GLOVE_API int hex_partner_config_init(HexPartnerConfig* config);
/* Borrows an idle glove, its mocap handle, and optionally an external snapshot
 * handle using the same pair. Do not separately start/abort/update these
 * handles while the controller is active. One caller thread owns every call. */
HEX_GLOVE_API HexPartnerHandle hex_partner_create(HexGloveHandle glove, HexMocapHandle mocap,
                                                   HexExternalHandle external,
                                                   const HexPartnerConfig* config);
HEX_GLOVE_API void hex_partner_destroy(HexPartnerHandle handle);
HEX_GLOVE_API int hex_partner_connect(HexPartnerHandle handle, int64_t monotonic_ms);
HEX_GLOVE_API int hex_partner_disconnect(HexPartnerHandle handle);
/* Writes start(0)/stop(1) to an open receiver. Success confirms serial write,
 * not a device acknowledgement; observe STREAMING/PAUSED and fresh mask. */
HEX_GLOVE_API int hex_partner_stream_control(HexPartnerHandle handle, int start);
HEX_GLOVE_API int hex_partner_tick(HexPartnerHandle handle, int64_t monotonic_ms);
HEX_GLOVE_API int hex_partner_get_status(HexPartnerHandle handle, HexPartnerStatus* output);
HEX_GLOVE_API int hex_partner_get_timing(HexPartnerHandle handle, HexPartnerTiming* output);
/* Returns 1 only when a newer event exists, 2 when unchanged, 0 for bad args. */
HEX_GLOVE_API int hex_partner_get_event(HexPartnerHandle handle, uint32_t after_sequence,
                                        HexPartnerEvent* output);
HEX_GLOVE_API const char* hex_partner_error_string(uint32_t code);

/* R is simultaneous for both hands when mask=3. Full mode additionally uses
 * the existing guided calibration flow; the host advances prompted steps. */
HEX_GLOVE_API int hex_partner_calibration_begin(HexPartnerHandle handle, uint32_t hand_mask,
                                                 uint32_t mode, float r_seconds);
HEX_GLOVE_API int hex_partner_calibration_advance(HexPartnerHandle handle, int hand);
HEX_GLOVE_API int hex_partner_calibration_commit_ring(HexPartnerHandle handle, int hand,
    const float* measured_mm4, const float* model_mm4, float palm_measured, float palm_model);
HEX_GLOVE_API int hex_partner_calibration_cancel(HexPartnerHandle handle);
HEX_GLOVE_API int hex_partner_calibration_retry(HexPartnerHandle handle);
HEX_GLOVE_API int hex_partner_get_calibration(HexPartnerHandle handle,
                                               HexPartnerCalibrationStatus* output);

/* One .hexprofile stores both hands. File association uses receiver port plus
 * left/right slots; neither this key nor device_code is a physical unique ID.
 * Save/load use the configured directory and current selected port. */
HEX_GLOVE_API int hex_partner_profile_save(HexPartnerHandle handle);
HEX_GLOVE_API int hex_partner_profile_load(HexPartnerHandle handle);
HEX_GLOVE_API int hex_partner_profile_path(HexPartnerHandle handle, char* output,
                                           uint32_t capacity, uint32_t* required);
/* Exact legacy UDP JSON mapping: channel 0 angles, 1 pose, 2 position,
 * 3 rotation. Call after tick with a fresh frame and caller-supplied UTC ms.
 * Returns bytes including NUL, or 0 on failure; query size with NULL,0. */
HEX_GLOVE_API uint32_t hex_partner_encode_legacy_udp(HexPartnerHandle handle,
    uint32_t channel, int64_t utc_ms, char* output, uint32_t capacity);

#ifdef __cplusplus
}
#endif
