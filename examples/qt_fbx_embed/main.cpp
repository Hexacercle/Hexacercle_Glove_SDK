#include "hex_glove/hex_qt_fbx_view.h"
#include "hex_glove/hex_partner_api.h"

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QKeySequence>
#include <QLabel>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <array>

namespace {
struct Session {
    HexGloveHandle glove = nullptr;
    HexMocapHandle mocap = nullptr;
    HexExternalHandle frame = nullptr;
    HexPartnerHandle partner = nullptr;

    ~Session() {
        if (partner) {
            hex_partner_disconnect(partner);
            hex_partner_destroy(partner);
        }
        if (frame) hex_external_destroy(frame);
        if (mocap) hex_mocap_destroy(mocap);
        if (glove) hex_glove_destroy(glove);
    }
};
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const auto args = app.arguments();
    const bool offline = args.size() == 1;
    const bool wireless = args.size() == 2 && !args[1].startsWith('-');
    const bool wired = args.size() == 4 && args[1] == "--wired";
    if (!offline && !wireless && !wired) {
        qInfo("qt_fbx_embed [COM3 | --wired COM12 COM14]");
        return args.size() == 2 && args[1] == "--help" ? 0 : 1;
    }

    QWidget window;
    window.setWindowTitle(QStringLiteral("HexaCercle Qt FBX 示例"));
    window.resize(850, 530);
    auto* layout = new QVBoxLayout(&window);
    QWidget* view = hex_qt_fbx_create(&window);
    if (!view) return 2;
    layout->addWidget(view, 1);
    auto* status = new QLabel(offline ? QStringLiteral("离线预览") : QStringLiteral("等待手套数据"), &window);
    layout->addWidget(status);
    if (offline) {
        window.show();
        return app.exec();
    }

    Session sdk;
    sdk.glove = hex_glove_create();
    if (!sdk.glove) return 3;
    sdk.mocap = hex_mocap_create(sdk.glove);
    if (!sdk.mocap || !hex_qt_fbx_configure_default_model(sdk.mocap)) return 3;
    sdk.frame = hex_external_create(sdk.glove, sdk.mocap);
    HexPartnerConfig config{};
    config.struct_size = sizeof(config);
    if (!sdk.frame || !hex_partner_config_init(&config)) return 3;
    config.auto_detect = config.auto_load_profile = config.auto_save_profile = 0;
    if (wired) {
        const auto left = args[2].toUtf8();
        const auto right = args[3].toUtf8();
        sdk.partner = hex_partner_create_wired(sdk.glove, sdk.mocap, sdk.frame, &config,
                                              left.constData(), right.constData());
    } else {
        const auto port = args[1].toUtf8();
        if (size_t(port.size()) >= sizeof(config.preferred_port_utf8)) return 1;
        qstrncpy(config.preferred_port_utf8, port.constData(), sizeof(config.preferred_port_utf8));
        sdk.partner = hex_partner_create(sdk.glove, sdk.mocap, sdk.frame, &config);
    }
    QElapsedTimer clock;
    clock.start();
    if (!sdk.partner || !hex_partner_connect(sdk.partner, clock.elapsed())) return 4;

    std::array<int, 2> layouts{11, 11};
    uint32_t streaming = 0;
    uint32_t calibrated = 0;
    uint32_t shown = 0;
    bool capturing = false;
    auto* quickR = new QShortcut(QKeySequence(QStringLiteral("R")), &window);
    QObject::connect(quickR, &QShortcut::activated, &window, [&] {
        if (streaming && !capturing && hex_qt_fbx_start_quick_r(sdk.mocap, int(streaming))) {
            capturing = true;
            calibrated &= ~streaming;
            status->setText(QStringLiteral("快速 R 采集中，请保持平掌伸直"));
        }
    });

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &window, [&] {
        // The partner controller alone advances the SDK and populates the cached frame.
        hex_partner_tick(sdk.partner, clock.elapsed());
        HexPartnerStatus current{};
        current.struct_size = sizeof(current);
        if (!hex_partner_get_status(sdk.partner, &current)) return;
        const uint32_t oldStreaming = streaming;
        streaming = current.streaming_hand_mask & 3u;
        if (capturing) {
            HexMocapRCaptureInfo info{};
            info.struct_size = sizeof(info);
            if (hex_mocap_get_r_capture_info(sdk.mocap, &info) && info.state != HEX_MOCAP_R_SAMPLING) {
                capturing = false;
                if (info.state == HEX_MOCAP_R_COMPLETE) calibrated |= uint32_t(info.hand_mask);
                status->setText(info.state == HEX_MOCAP_R_COMPLETE ? QStringLiteral("快速 R 完成")
                    : QStringLiteral("快速 R 未完成，可再次按 R"));
            }
        } else if (streaming != oldStreaming) {
            status->setText(streaming ? QStringLiteral("平掌伸直，按 R 快速校准")
                                     : QStringLiteral("等待手套数据"));
        }

        HexExternalFrame frame{};
        frame.struct_size = sizeof(frame);
        if (!hex_external_get_frame(sdk.frame, &frame)) frame.tracked_hand_mask = shown;
        for (int hand = 0; hand < 2; ++hand) {
            const uint32_t bit = 1u << hand;
            if (!capturing && (current.fresh_hand_mask & bit)) {
                const int layout = hex_qt_fbx_get_input_layout(sdk.mocap, hand);
                if ((layout == 7 || layout == 11) && layout != layouts[hand] &&
                    hex_qt_fbx_set_input_layout(sdk.mocap, hand, layout)) {
                    layouts[hand] = layout;
                    calibrated &= ~bit;
                    shown &= ~bit;
                    hex_qt_fbx_set_pose(view, hand, nullptr);
                    status->setText(QStringLiteral("输入布局已切换，平掌伸直后按 R"));
                    continue;
                }
            }
            if (streaming & calibrated & frame.tracked_hand_mask & bit) {
                if ((frame.fresh_hand_mask & bit) && frame.pose[hand].tracked_mask &&
                    hex_qt_fbx_set_pose(view, hand, &frame.pose[hand]))
                    shown |= bit;
            } else if (shown & bit) {
                hex_qt_fbx_set_pose(view, hand, nullptr);
                shown &= ~bit;
            }
        }
    });
    timer.setTimerType(Qt::PreciseTimer);
    timer.start(10);
    window.show();
    return app.exec();
}
