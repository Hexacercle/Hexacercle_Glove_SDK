#include "hex_glove/hex_glove_api.h"
#include "hex_glove/hex_mocap_api.h"
#include "hex_glove/hex_external_api.h"
#include "hex_glove/hex_partner_api.h"
#include "hex_glove/hex_hardware_info_api.h"
#include "hex_glove/hex_pipeline_api.h"
#include "unity_mapping.h"
#include "../angle_output.h"
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QByteArray>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTextOption>
#include <QSizePolicy>
#include <QShortcut>
#include <QSignalBlocker>
#include <cmath>
#include <utility>

// These exports are declared in the SDK's internal headers.
extern "C" {
HEX_GLOVE_API int hex_mocap_start_r_capture_remount(HexMocapHandle handle, int hand_mask, float seconds);
HEX_GLOVE_API int hex_mocap_cancel_r(HexMocapHandle handle);
HEX_GLOVE_API int hex_mocap_set_legacy_live_recovery(HexMocapHandle handle, int enabled);
HEX_GLOVE_API int hex_mocap_get_hand_driver_state(HexMocapHandle handle, int hand, HexMocapHandDriverState* state);
HEX_GLOVE_API int hex_glove_list_ports(char* buffer, uint32_t capacity, uint32_t* required);
}

namespace {
constexpr float quickRSeconds = 2.0f;
constexpr qint64 quickRTimeoutMs = 8000;
constexpr int renderIntervalMs = 16;
constexpr qint64 textIntervalMs = 100;
constexpr uint32_t udpBufferSize = 65508;

struct ConnectionSelection {
    bool wired = false;
    QString receiver;
    QString left;
    QString right;

    bool operator==(const ConnectionSelection& other) const {
        return wired == other.wired && receiver == other.receiver
            && left == other.left && right == other.right;
    }

    QString description() const {
        if (!wired) return QStringLiteral("无线 · 接收器 %1").arg(receiver);
        const auto omitted = QStringLiteral("未选择");
        return QStringLiteral("有线 · 左手 %1 · 右手 %2")
            .arg(left.isEmpty() ? omitted : left, right.isEmpty() ? omitted : right);
    }
};

QString normalizedPort(QString port) {
    port = port.trimmed();
#ifdef Q_OS_WIN
    port = port.toUpper();
#endif
    return port;
}

QStringList availablePorts() {
    uint32_t required = 0;
    if (!hex_glove_list_ports(nullptr, 0, &required) || required < 2) return {};
    QByteArray names(int(required), '\0');
    if (!hex_glove_list_ports(names.data(), required, &required)) return {};
    auto ports = QString::fromUtf8(names.constData()).split('\n');
    ports.removeAll(QString());
    return ports;
}

struct ViewerSession {
    HexGloveHandle glove{};
    HexMocapHandle mocap{};
    HexExternalHandle external{};
    HexPartnerHandle partner{};

    ViewerSession() = default;
    ViewerSession(const ViewerSession&) = delete;
    ViewerSession& operator=(const ViewerSession&) = delete;
    ~ViewerSession() {
        if (partner) hex_partner_destroy(partner);
        if (external) hex_external_destroy(external);
        if (mocap) hex_mocap_destroy(mocap);
        if (glove) hex_glove_destroy(glove);
    }

    int initialize(const char* profile, bool oppositionAssist) {
        glove = hex_glove_create();
        if (!glove) return 2;
        mocap = hex_mocap_create(glove);
        if (!mocap) return 3;
        const bool initialized = profile
            ? hex_mocap_profile_load(mocap, profile) != 0 : installUnityMapping(mocap);
        if (!initialized || !hex_mocap_set_legacy_live_recovery(mocap, 1)) return 4;
#ifdef Q_OS_WIN
        if (!hex_mocap_set_opposition_assist_enabled(mocap, oppositionAssist ? 1 : 0)) return 4;
#else
        Q_UNUSED(oppositionAssist);
#endif
        external = hex_external_create(glove, mocap);
        return external ? 0 : 5;
    }

    void swap(ViewerSession& other) {
        std::swap(glove, other.glove);
        std::swap(mocap, other.mocap);
        std::swap(external, other.external);
        std::swap(partner, other.partner);
    }
};

QString phaseName(uint32_t phase) {
    const QStringList names{QStringLiteral("已停止"), QStringLiteral("搜索中"),
        QStringLiteral("连接中"), QStringLiteral("等待数据"), QStringLiteral("数据输出中"),
        QStringLiteral("数据中断"), QStringLiteral("重连中"), QStringLiteral("连接失败"),
        QStringLiteral("数据流已暂停")};
    return phase < uint32_t(names.size()) ? names[int(phase)] : QStringLiteral("未知状态");
}
QString operationName(uint32_t state) {
    const QStringList names{QStringLiteral("空闲"), QStringLiteral("进行中"),
        QStringLiteral("成功"), QStringLiteral("失败"), QStringLiteral("已取消")};
    return state < uint32_t(names.size()) ? names[int(state)] : QStringLiteral("未知");
}
QString errorName(uint32_t error) {
    const QStringList names{QStringLiteral("无"), QStringLiteral("参数无效"),
        QStringLiteral("结构大小不匹配"), QStringLiteral("接收器打开或探测失败，请检查串口、驱动或占用"),
        QStringLiteral("接收器连接丢失"), QStringLiteral("数据接收超时"), QStringLiteral("数据更新失败"),
        QStringLiteral("无法开始基准校准"), QStringLiteral("基准校准失败"), QStringLiteral("完整标定失败"),
        QStringLiteral("未找到校准档案"), QStringLiteral("校准档案保存失败"), QStringLiteral("校准档案加载失败"),
        QStringLiteral("尚未绑定接收器端口"), QStringLiteral("尚无新鲜数据帧"), QStringLiteral("协议数据编码失败"),
        QStringLiteral("串口不存在"), QStringLiteral("串口访问被拒绝或被占用"),
        QStringLiteral("串口配置失败"), QStringLiteral("启动流命令写入失败"),
        QStringLiteral("数据流命令写入失败")};
    return error < uint32_t(names.size()) ? names[int(error)]
        : QStringLiteral("未知错误（%1）").arg(error);
}
QString eventName(uint32_t kind) {
    const QStringList names{QStringLiteral("状态更新"), QStringLiteral("接收器已连接"),
        QStringLiteral("开始输出数据"), QStringLiteral("数据中断"), QStringLiteral("正在重连"),
        QStringLiteral("操作失败"), QStringLiteral("已断开连接"), QStringLiteral("校准档案已加载"),
        QStringLiteral("基准校准完成"), QStringLiteral("完整标定完成"), QStringLiteral("标定失败"),
        QStringLiteral("数据流已暂停"), QStringLiteral("数据流命令失败")};
    return kind < uint32_t(names.size()) ? names[int(kind)] : QStringLiteral("状态更新");
}
QString handNames(uint32_t mask) {
    if ((mask & 3u) == 3u) return QStringLiteral("双手");
    if (mask & 1u) return QStringLiteral("左手");
    if (mask & 2u) return QStringLiteral("右手");
    return QStringLiteral("无");
}

#ifdef Q_OS_WIN
QString endpointPrompt(const HexMocapContactEndpointStatus& status) {
    switch (status.phase) {
    case HEX_MOCAP_ENDPOINT_PREPARING: return QStringLiteral("准备指腹捏合");
    case HEX_MOCAP_ENDPOINT_PINCH: return QStringLiteral("指腹轻贴并保持");
    case HEX_MOCAP_ENDPOINT_CHANGE: return QStringLiteral("捏合已保存，可以松开并换成指尖掐合");
    case HEX_MOCAP_ENDPOINT_CLAW: return QStringLiteral("指尖轻贴并保持");
    case HEX_MOCAP_ENDPOINT_READY:
    case HEX_MOCAP_ENDPOINT_VALIDATING: return QStringLiteral("两姿态已采集，等待验证");
    case HEX_MOCAP_ENDPOINT_LITTLE_STANDARD:
    case HEX_MOCAP_ENDPOINT_FAILED: return QStringLiteral("采集或验证暂停，请检查姿势后重试");
    default: return QStringLiteral("等待采集");
    }
}
#endif
} // namespace

class SkeletonCanvas : public QWidget {
public:
    // 待机姿态和实时姿态分别保存，避免把显示预览当成跟踪数据。
    HexMocapDisplayPose pose[2]{};
    HexMocapDisplayPose preview[2]{};
    bool streaming[2]{};
    bool hasBaseline[2]{};
    int parent[2][21]{};
    bool wristXyEnabled = false;
    bool viewBasisValid[2]{};
    HexMocapVec3 viewUp[2]{}, viewRight[2]{};
    SkeletonCanvas() { setMinimumSize(440, 260); setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); }
    static HexMocapVec3 subtract(HexMocapVec3 a,HexMocapVec3 b) {
        return {a.x-b.x,a.y-b.y,a.z-b.z};
    }
    static float dot(HexMocapVec3 a,HexMocapVec3 b) {
        return a.x*b.x+a.y*b.y+a.z*b.z;
    }
    static bool normalize(HexMocapVec3& v) {
        const float length=std::sqrt(dot(v,v));
        if(length<1e-5f) return false;
        v={v.x/length,v.y/length,v.z/length};
        return true;
    }
    void resetViewBasis() { viewBasisValid[0]=viewBasisValid[1]=false; }
    const HexMocapDisplayPose& visiblePose(int hand) const {
        return hasBaseline[hand] && pose[hand].tracked_mask ? pose[hand] : preview[hand];
    }
    bool frontViewBasis(int hand, const HexMocapDisplayPose& p,
                        HexMocapVec3& up, HexMocapVec3& right) const {
        const uint32_t required=(1u<<0)|(1u<<5)|(1u<<9)|(1u<<17);
        if((p.valid_mask&required)!=required) return false;
        // 每只手分别按实时掌面定正面：中指根部朝上，拇指朝画面外侧。
        up=subtract(p.world_position[9],p.world_position[0]);
        right=hand?subtract(p.world_position[5],p.world_position[17]):
                   subtract(p.world_position[17],p.world_position[5]);
        if(!normalize(up)) return false;
        const float along=dot(right,up);
        right={right.x-along*up.x,right.y-along*up.y,right.z-along*up.z};
        return normalize(right);
    }
    void captureViewBasis(int hand) {
        if(viewBasisValid[hand]) return;
        HexMocapVec3 up{},right{};
        if(!frontViewBasis(hand,visiblePose(hand),up,right)) return;
        viewUp[hand]=up; viewRight[hand]=right; viewBasisValid[hand]=true;
    }
protected:
    void paintEvent(QPaintEvent*) override {
        // SDK 给出 Unity 世界位置。锁定时以实时掌面正对画面，解锁时保持捕获的观察轴以显示腕部运动。
        QPainter painter(this);
        painter.fillRect(rect(), QColor(25, 29, 38));
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::white);
        painter.drawText(16, 25, wristXyEnabled ? "实时骨骼 · 腕部 XY 已开启（F5/F6 切换，F8 正面重置）" :
            "实时骨骼 · 腕部 XY 已锁定（F5/F6 切换，F8 正面重置）");
        for (int hand = 0; hand < 2; ++hand) {
            const auto& p = visiblePose(hand);
            const auto mask = p.tracked_mask ? p.tracked_mask : p.valid_mask;
            const QColor color = hand ? QColor(255, 171, 92) : QColor(89, 203, 255);
            const QPointF origin(width() * (hand ? .72 : .28), height() * .67);
            painter.setPen(color);
            painter.drawText(QPointF(origin.x() - 60, 52), hand ? "右手 · SDK 1" : "左手 · SDK 0");
            const QString tracking = p.tracked_mask
                ? (streaming[hand] ? QStringLiteral("正在跟踪") : QStringLiteral("等待新帧"))
                : (streaming[hand] && !hasBaseline[hand] ? QStringLiteral("等待 R 快速校准") : QStringLiteral("待机姿态"));
            painter.drawText(QPointF(origin.x() - 60, 72), tracking);
            if (!mask) continue;
            HexMocapVec3 up=viewUp[hand],right=viewRight[hand];
            if(!wristXyEnabled) {
                // 锁定腕部时始终让掌心朝向画面；仅改变预览投影，不改 SDK 骨骼数据。
                if(frontViewBasis(hand,p,up,right)) {
                    viewUp[hand]=up; viewRight[hand]=right; viewBasisValid[hand]=true;
                } else if(!viewBasisValid[hand]) continue;
            } else if(!viewBasisValid[hand]) continue;
            auto project = [&](int i) {
                const auto& v = p.world_position[i];
                const auto& palm = p.world_position[0];
                // 解锁后沿固定观察轴投影，保留腕部运动；锁定时使用当前掌面轴。
                const auto offset=subtract(v,palm);
                const double scale=qMin(1300.0,height()*3.0);
                return QPointF(origin.x()+dot(offset,right)*scale,
                    origin.y()-dot(offset,up)*scale);
            };
            painter.setPen(QPen(color, 3));
            for (int i = 1; i < 21; ++i) {
                const int j = parent[hand][i];
                if (j < 0 || j >= 21) continue;
                if (!(mask & (1u << i)) || !(mask & (1u << j))) continue;
                painter.drawLine(project(j), project(i));
            }
            painter.setBrush(color);
            for (int i = 0; i < 21; ++i)
                if (mask & (1u << i)) painter.drawEllipse(project(i), 4, 4);
        }
    }
};

int main(int argc, char** argv) {
    // 解析串口、校准档案与可选的 CSV/JSON Lines 输出路径。
    QApplication app(argc, argv);
    ConnectionSelection initialConnection;
    const char* profile = nullptr;
    bool noProfile = false;
    const char* csv_path = nullptr;
    const char* jsonl_path = nullptr;
    for (int i = 1; i < argc; ++i) {
        const QString option = QString::fromUtf8(argv[i]);
        if (option == "--profile" && i + 1 < argc) profile = argv[++i];
        else if (option == "--no-profile") noProfile = true;
        else if (option == "--csv" && i + 1 < argc) csv_path = argv[++i];
        else if (option == "--jsonl" && i + 1 < argc) jsonl_path = argv[++i];
        else if (option == "--wired") {
#ifdef Q_OS_WIN
            if (initialConnection.wired || i + 2 >= argc) {
                qWarning("--wired 需要两个不同的串口，顺序为左手、右手。");
                return 1;
            }
            initialConnection.wired = true;
            initialConnection.left = normalizedPort(QString::fromUtf8(argv[++i]));
            initialConnection.right = normalizedPort(QString::fromUtf8(argv[++i]));
#else
            qWarning("本发布包的有线双串口模式仅支持 Windows。");
            return 1;
#endif
        } else if (option == "--help" || option == "-h") {
            qInfo("用法：qt_skeleton_viewer [接收器串口 | --wired 左手串口 右手串口（Windows）] "
                  "[--profile 校准文件 | --no-profile] [--csv 输出文件] [--jsonl 输出文件|-]");
            return 0;
        } else if (!option.startsWith('-') && initialConnection.receiver.isEmpty()) {
            initialConnection.receiver = normalizedPort(option);
        } else {
            qWarning("未知参数：%s", argv[i]);
            return 1;
        }
    }
    if (initialConnection.wired && (!initialConnection.receiver.isEmpty() ||
        initialConnection.left.isEmpty() || initialConnection.right.isEmpty() ||
        initialConnection.left == initialConnection.right ||
        initialConnection.left.startsWith('-') || initialConnection.right.startsWith('-'))) {
        qWarning("--wired 需要两个不同的串口，不能同时指定无线接收器串口。");
        return 1;
    }
    if (profile && noProfile) {
        qWarning("--profile 和 --no-profile 只能选择一个。");
        return 1;
    }
    AngleOutput angles;
    // AngleOutput 封装左右手 Hub 零位、逐帧角度和带时间戳的文件输出。
    if (!angles.open(csv_path, jsonl_path)) { qWarning("无法打开角度输出文件"); return 1; }
    ViewerSession session;
    bool oppositionAssist = false;
    const int initializationResult = session.initialize(profile, oppositionAssist);
    if (initializationResult) {
        if (profile && initializationResult == 4) qWarning("无法初始化校准文件：%s", profile);
        else qWarning("无法初始化手套会话和 Unity 骨架");
        return initializationResult;
    }
    auto& glove = session.glove;
    auto& mocap = session.mocap;
    auto& partner = session.partner;
    HexPartnerConfig cfg{}; cfg.struct_size = sizeof(cfg);
    if (!hex_partner_config_init(&cfg)) return 5;
    cfg.auto_detect = 0;
    if (profile || noProfile) cfg.auto_load_profile = 0;
    if (noProfile) {
        cfg.auto_save_profile = 0;
    } else {
        const QByteArray profileDir = (QCoreApplication::applicationDirPath() + "/profiles").toUtf8();
        qstrncpy(cfg.profile_dir_utf8, profileDir.constData(), sizeof(cfg.profile_dir_utf8));
    }
    ConnectionSelection activeConnection;
    QElapsedTimer clock;
    clock.start();
    QWidget window;
    window.resize(1180, 800);
    window.setMinimumSize(880, 620);
    window.setStyleSheet(QStringLiteral(
        "QWidget { font-size: 14px; }"
        "QGroupBox { font-weight: bold; border: 1px solid #d8dee7; border-radius: 6px; margin-top: 12px; padding: 12px 8px 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }"
        "QPushButton { min-height: 30px; padding: 2px 8px; }"
        "QPushButton#primary { background: #177f86; color: white; border: 1px solid #12666c; border-radius: 4px; }"
        "QTabWidget::pane { border: 1px solid #d8dee7; }"
        "QPlainTextEdit { background: #f7f9fc; border: none; padding: 8px; }"));
    auto* layout = new QHBoxLayout(&window);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);
    auto* sidebar = new QWidget;
    sidebar->setFixedWidth(224);
    auto* controls = new QVBoxLayout(sidebar);
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(10);
    auto* heading = new QLabel(QStringLiteral("手套控制"));
    heading->setStyleSheet("font-size: 20px; font-weight: bold; color: #12666c;");
    controls->addWidget(heading);
    auto* portLabel = new QLabel(QStringLiteral("未连接"));
    portLabel->setWordWrap(true);
    controls->addWidget(portLabel);
    auto* reconnect = new QPushButton(QStringLiteral("连接手套"));
    auto* streamButton = new QPushButton(QStringLiteral("暂停数据流"));
    auto* r = new QPushButton(QStringLiteral("R 快速校准（2 秒）"));
    auto* full = new QPushButton(QStringLiteral("开始双手七步校准"));
    full->setObjectName("primary");
    auto* retryContact = new QPushButton(QStringLiteral("重试当前对指"));
    auto* cancelR = new QPushButton(QStringLiteral("取消标定"));
    auto* retryR = new QPushButton(QStringLiteral("重新标定"));
    auto* saveProfile = new QPushButton(QStringLiteral("保存校准档案"));
    auto* loadProfile = new QPushButton(QStringLiteral("加载校准档案"));
    saveProfile->setEnabled(!noProfile);
    loadProfile->setEnabled(!noProfile);
    auto* angle_zero = new QPushButton(QStringLiteral("角度归零（手指伸直）"));
    auto* connectionGroup = new QGroupBox(QStringLiteral("设备连接"));
    auto* connectionLayout = new QVBoxLayout(connectionGroup);
    auto* connectionMode = new QComboBox;
    connectionMode->addItem(QStringLiteral("无线（单接收器）"));
#ifdef Q_OS_WIN
    connectionMode->addItem(QStringLiteral("有线（双串口）"));
#endif
    connectionMode->setAccessibleName(QStringLiteral("连接方式"));
    connectionMode->setCurrentIndex(initialConnection.wired ? 1 : 0);
    connectionLayout->addWidget(connectionMode);
    auto makePortSelector = [&](const QString& labelText, const QString& initialPort) {
        auto* label = new QLabel(labelText);
        auto* selector = new QComboBox;
        selector->setEditable(true);
        selector->setInsertPolicy(QComboBox::NoInsert);
        selector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        selector->setAccessibleName(labelText);
        selector->lineEdit()->setPlaceholderText(QStringLiteral("选择或输入串口"));
        selector->setEditText(initialPort);
        label->setBuddy(selector);
        connectionLayout->addWidget(label);
        connectionLayout->addWidget(selector);
        return std::pair{label, selector};
    };
    const auto [receiverLabel, receiverPort] = makePortSelector(
        QStringLiteral("无线接收器串口"), initialConnection.receiver);
    const auto [leftLabel, leftPort] = makePortSelector(QStringLiteral("左手串口"), initialConnection.left);
    const auto [rightLabel, rightPort] = makePortSelector(QStringLiteral("右手串口"), initialConnection.right);
    auto* refreshPortsButton = new QPushButton(QStringLiteral("刷新串口"));
    connectionLayout->addWidget(refreshPortsButton);
    connectionLayout->addWidget(reconnect);
    connectionLayout->addWidget(streamButton);
    auto refreshPorts = [&] {
        const auto names = availablePorts();
        for (auto* selector : {receiverPort, leftPort, rightPort}) {
            const auto current = selector->currentText();
            selector->clear();
            selector->addItems(names);
            selector->setCurrentIndex(selector->findText(current));
            selector->setEditText(current);
        }
    };
    auto showSelectedTransport = [&] {
        const bool wired = connectionMode->currentIndex() == 1;
        receiverLabel->setVisible(!wired);
        receiverPort->setVisible(!wired);
        leftLabel->setVisible(wired);
        leftPort->setVisible(wired);
        rightLabel->setVisible(wired);
        rightPort->setVisible(wired);
    };
    QObject::connect(connectionMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &window, showSelectedTransport);
    QObject::connect(refreshPortsButton, &QPushButton::clicked, &window, refreshPorts);
    refreshPorts();
    showSelectedTransport();
    controls->addWidget(connectionGroup);
    auto* calibrationGroup = new QGroupBox(QStringLiteral("姿态标定"));
    auto* calibrationLayout = new QGridLayout(calibrationGroup);
    calibrationLayout->addWidget(r, 0, 0, 1, 2);
    calibrationLayout->addWidget(full, 1, 0, 1, 2);
    calibrationLayout->addWidget(retryContact, 2, 0, 1, 2);
    calibrationLayout->addWidget(cancelR, 3, 0);
    calibrationLayout->addWidget(retryR, 3, 1);
    calibrationLayout->addWidget(angle_zero, 4, 0, 1, 2);
#ifdef Q_OS_WIN
    auto* assistBox = new QCheckBox(QStringLiteral("对指吸附"));
    assistBox->setToolTip(QStringLiteral("对双手实时回放生效，本次运行内保留选择。"));
    calibrationLayout->addWidget(assistBox, 5, 0, 1, 2);
    auto* recapture = new QPushButton(QStringLiteral("重新采集当前手指两姿态"));
    recapture->setEnabled(false);
    calibrationLayout->addWidget(recapture, 6, 0, 1, 2);
#endif
    controls->addWidget(calibrationGroup);
    auto* profileGroup = new QGroupBox(QStringLiteral("校准档案"));
    auto* profileLayout = new QVBoxLayout(profileGroup);
    auto* profileSource = new QLabel(profile
        ? QStringLiteral("当前：%1").arg(QFileInfo(QString::fromUtf8(profile)).fileName())
        : noProfile ? QStringLiteral("Unity 初始映射 · 档案读写已停用")
                    : QStringLiteral("Unity 初始映射 · 等待串口档案"));
    const auto initialProfileSource = profileSource->text();
    profileSource->setWordWrap(true);
    profileLayout->addWidget(profileSource);
    profileLayout->addWidget(saveProfile);
    profileLayout->addWidget(loadProfile);
    controls->addWidget(profileGroup);
    auto* hint = new QLabel(QStringLiteral("七步校准由 SDK 自动推进双手：平放、伸直、握拳、拇指依次与四指捏→掐。F5/F6 与 Unity F6 一样切换腕部 XY 同步；F8 按当前双手掌面重置正面视角。真实校准请先加载 Unity 导出的模型配置。"));
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #606d7c; padding: 6px 2px;");
    controls->addWidget(hint);
    controls->addStretch();
    auto* sidebarScroll = new QScrollArea;
    sidebarScroll->setWidget(sidebar);
    sidebarScroll->setWidgetResizable(true);
    sidebarScroll->setFrameShape(QFrame::NoFrame);
    sidebarScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarScroll->setFixedWidth(244);
    layout->addWidget(sidebarScroll);

    auto* right = new QVBoxLayout;
    right->setSpacing(10);
    layout->addLayout(right, 1);
    auto* status = new QLabel(QStringLiteral("正在连接手套，请稍候…"));
    status->setWordWrap(true);
    status->setTextFormat(Qt::PlainText);
    status->setStyleSheet("background: #e9f4f4; color: #164d52; padding: 10px; border-radius: 6px;");
    right->addWidget(status);
    auto* splitter = new QSplitter(Qt::Vertical);
    auto* canvas = new SkeletonCanvas;
    splitter->addWidget(canvas);
    auto* outputTabs = new QTabWidget;
    outputTabs->setMinimumHeight(160);
    auto makeText = [] {
        auto* text = new QPlainTextEdit;
        text->setReadOnly(true);
        text->setWordWrapMode(QTextOption::WrapAnywhere);
        text->setMinimumWidth(0);
        return text;
    };
    auto* angle_text = makeText();
    const auto angleInstructions = QStringLiteral("完成基准校准后，伸直手指并点击左侧“角度归零”。\n角度单位：度。");
    angle_text->setPlainText(angleInstructions);
    bool angleSampleReady[2]{};
    bool showingAngleSamples = false;
    auto* calibrationText = new QLabel(QStringLiteral("双手平放、手指自然伸直后，点击左侧「R 快速校准」。"));
    calibrationText->setWordWrap(true);
    calibrationText->setTextFormat(Qt::PlainText);
    calibrationText->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    calibrationText->setStyleSheet("padding: 12px;");
    auto* calibrationScroll = new QScrollArea;
    calibrationScroll->setWidgetResizable(true);
    calibrationScroll->setWidget(calibrationText);
    calibrationScroll->setFrameShape(QFrame::NoFrame);
    auto* eventText = makeText();
    eventText->setMaximumBlockCount(300);
    auto* udpText = makeText();
    udpText->setLineWrapMode(QPlainTextEdit::NoWrap);
    const auto udpWaitingText = QStringLiteral("等待新鲜数据帧。协议原始字段名保持不变，便于与既有数据对接。");
    udpText->setPlainText(udpWaitingText);
    outputTabs->addTab(angle_text, QStringLiteral("实时角度"));
    outputTabs->addTab(calibrationScroll, QStringLiteral("标定进度"));
    outputTabs->addTab(eventText, QStringLiteral("事件记录"));
    outputTabs->addTab(udpText, QStringLiteral("协议原始数据"));
    splitter->addWidget(outputTabs);
    splitter->setChildrenCollapsible(false);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({430, 240});
    right->addWidget(splitter, 1);
    auto notify = [&](const QString& message) {
        eventText->appendPlainText(QDateTime::currentDateTime().toString("HH:mm:ss") + "  " + message);
    };
#ifdef Q_OS_WIN
    QObject::connect(assistBox, &QCheckBox::toggled, &window, [&](bool enabled) {
        if (hex_mocap_set_opposition_assist_enabled(mocap, enabled ? 1 : 0)) {
            oppositionAssist = enabled;
        } else {
            const QSignalBlocker block(assistBox);
            assistBox->setChecked(oppositionAssist);
            notify(QStringLiteral("对指吸附设置失败，请检查 SDK 会话。"));
        }
    });
#endif
    bool usesUnityDefaults = !profile;
    int inputLayouts[2] = {11, 11};
    bool canvasDirty = true;
    auto refreshPreviews = [&] {
        canvas->resetViewBasis();
        for (int hand = 0; hand < 2; ++hand) {
            HexMocapSkeletonDefinition model{};
            model.struct_size = sizeof(model);
            if (hex_mocap_get_skeleton_definition(mocap, hand, &model)) {
                for (int node = 0; node < 21; ++node)
                    canvas->parent[hand][node] = model.parent[node];
            }
            auto& preview = canvas->preview[hand];
            preview = {};
            preview.struct_size = sizeof(preview);
            if (usesUnityDefaults) applyUnityIdlePreview(hand, preview);
            else hex_mocap_get_display_pose(mocap, hand, &preview);
            preview.tracked_mask = 0;
            canvas->pose[hand] = {};
        }
        canvasDirty = true;
    };
    refreshPreviews();
    uint32_t quickRMask = 0;
    qint64 quickRDeadline = 0;
    auto finishQuickR = [&](const QString& message) {
        quickRMask = 0;
        r->setEnabled(true);
        calibrationText->setText(message);
        notify(message);
    };
    auto cancelQuickR = [&](const QString& message) {
        if (!quickRMask) return;
        hex_mocap_cancel_r(mocap);
        finishQuickR(message);
    };
    bool sevenActive = false;
    bool sevenSaved = false;
    bool sevenFinished = false;
    bool sevenFailureNotified = false;
    uint32_t eventSequence = 0;
    const QStringList sevenTitles{QStringLiteral("平放校准"), QStringLiteral("四指伸直"),
        QStringLiteral("四指握拳"), QStringLiteral("拇指与食指"), QStringLiteral("拇指与中指"),
        QStringLiteral("拇指与无名指"), QStringLiteral("拇指与小指")};
    auto selectedConnection = [&] {
        ConnectionSelection selection;
        selection.wired = connectionMode->currentIndex() == 1;
        if (selection.wired) {
            selection.left = normalizedPort(leftPort->currentText());
            selection.right = normalizedPort(rightPort->currentText());
        } else {
            selection.receiver = normalizedPort(receiverPort->currentText());
        }
        return selection;
    };
    auto createPartner = [&](ViewerSession& target, const ConnectionSelection& selection) {
        auto portConfig = cfg;
        const auto preferredPort = (selection.wired ? selection.left : selection.receiver).toUtf8();
        if (size_t(preferredPort.size()) >= sizeof(portConfig.preferred_port_utf8))
            return HexPartnerHandle{};
        qstrncpy(portConfig.preferred_port_utf8, preferredPort.constData(), sizeof(portConfig.preferred_port_utf8));
#ifdef Q_OS_WIN
        if (selection.wired) {
            const auto right = selection.right.toUtf8();
            return hex_partner_create_wired(target.glove, target.mocap, target.external,
                                            &portConfig, preferredPort.constData(), right.constData());
        }
#endif
        return hex_partner_create(target.glove, target.mocap, target.external, &portConfig);
    };
    auto startConnection = [&](const ConnectionSelection& selection) {
        QString error;
        if (selection.wired && (selection.left.isEmpty() || selection.right.isEmpty()))
            error = QStringLiteral("请选择左手和右手串口。");
        else if (selection.wired && selection.left == selection.right)
            error = QStringLiteral("左右手有线串口必须不同。");
        else if (!selection.wired && selection.receiver.isEmpty())
            error = QStringLiteral("请选择无线接收器串口。");
        if (!error.isEmpty()) {
            notify(error);
            outputTabs->setCurrentIndex(2);
            return false;
        }

        // 不同端口组合使用独立解算会话，避免沿用上一台设备的佩戴零位。
        if ((activeConnection.wired || !activeConnection.receiver.isEmpty()) &&
            !(activeConnection == selection)) {
            ViewerSession next;
            if (next.initialize(profile, oppositionAssist) || !(next.partner = createPartner(next, selection)) ||
                !hex_partner_connect(next.partner, clock.elapsed())) {
                notify(QStringLiteral("无法初始化所选串口会话，请检查端口和校准档案。"));
                outputTabs->setCurrentIndex(2);
                return false;
            }
            session.swap(next);
            usesUnityDefaults = !profile;
            inputLayouts[0] = inputLayouts[1] = 11;
            for (int hand = 0; hand < 2; ++hand) {
                canvas->streaming[hand] = canvas->hasBaseline[hand] = false;
                angles.reset(hand);
            }
            eventSequence = 0;
            sevenActive = sevenSaved = sevenFinished = sevenFailureNotified = false;
            profileSource->setText(initialProfileSource);
            calibrationText->setText(QStringLiteral("双手平放、手指自然伸直后，点击左侧「R 快速校准」。"));
            refreshPreviews();
        } else {
            if (!partner) partner = createPartner(session, selection);
            if (!partner || !hex_partner_connect(partner, clock.elapsed())) {
                if (partner) {
                    hex_partner_destroy(partner);
                    partner = nullptr;
                    eventSequence = 0;
                }
                notify(QStringLiteral("无法启动所选串口会话。"));
                outputTabs->setCurrentIndex(2);
                return false;
            }
        }
        activeConnection = selection;
        notify(QStringLiteral("正在连接 %1").arg(activeConnection.description()));
        return true;
    };
    auto updateConnectionControls = [&](const HexPartnerStatus& current) {
        const bool active = current.phase != HEX_PARTNER_STOPPED && current.phase != HEX_PARTNER_FAILED;
        const bool connected = current.phase == HEX_PARTNER_WAITING_DATA
            || current.phase == HEX_PARTNER_STREAMING || current.phase == HEX_PARTNER_STREAM_PAUSED
            || current.phase == HEX_PARTNER_DATA_INTERRUPTED;
        const bool calibrating = sevenActive || quickRMask;
        connectionMode->setEnabled(!active);
        receiverPort->setEnabled(!active);
        leftPort->setEnabled(!active);
        rightPort->setEnabled(!active);
        refreshPortsButton->setEnabled(!active);
        reconnect->setText(active ? QStringLiteral("断开连接") : QStringLiteral("连接手套"));
        portLabel->setText(active ? activeConnection.description() : QStringLiteral("未连接"));
        streamButton->setEnabled(connected && !calibrating);
        r->setEnabled(current.streaming_hand_mask && !calibrating);
        full->setEnabled(current.streaming_hand_mask && !calibrating);
        retryContact->setEnabled(sevenActive);
#ifdef Q_OS_WIN
        assistBox->setEnabled(!calibrating);
        if (!sevenActive) recapture->setEnabled(false);
#endif
        cancelR->setEnabled(calibrating);
        retryR->setEnabled(current.streaming_hand_mask && !quickRMask);
        saveProfile->setEnabled(!noProfile && connected && !calibrating);
        loadProfile->setEnabled(!noProfile && connected && !calibrating);
        angle_zero->setEnabled(current.streaming_hand_mask && !calibrating);
    };
    auto beginSeven = [&] {
        if (quickRMask) { notify(QStringLiteral("请先完成或取消 R 快速校准")); return; }
        outputTabs->setCurrentIndex(1);
        HexPartnerStatus current{}; current.struct_size=sizeof(current);
        hex_partner_get_status(partner,&current);
        const uint32_t mask=current.streaming_hand_mask & 3u;
        if (!mask) { notify(QStringLiteral("没有有效手套数据，无法开始七步校准")); return; }
#ifdef Q_OS_WIN
        if (!hex_mocap_seven_step_begin_endpoints(mocap, int(mask))) {
            notify(QStringLiteral("两姿态七步校准无法开始。请确认所选手持续输出标准 11 通道数据，且使用配套骨架映射。"));
            return;
        }
#else
        if (!profile && !current.profile_loaded) {
            notify(QStringLiteral("请先加载与当前佩戴匹配的 Unity 校准档案"));
            return;
        }
        if (!hex_mocap_seven_step_begin_original_ui(mocap,int(mask))) {
            notify(QStringLiteral("七步校准无法开始：检查数据、模型及其他校准流程")); return;
        }
#endif
        for(int h=0;h<2;++h) if(mask&(1u<<h)) {
            HexMocapContactQuality quality{}; quality.struct_size=sizeof(quality);
            bool compatible = hex_mocap_get_contact_quality(mocap,h,&quality) != 0;
#ifdef Q_OS_WIN
            HexMocapContactEndpointStatus status{};
            status.struct_size = sizeof(status);
            compatible = compatible && quality.version == 17
                && hex_mocap_get_contact_endpoint_status(mocap, h, &status) && status.version == 1;
#else
            compatible = compatible && quality.version == 11;
#endif
            if (!compatible) {
                hex_mocap_seven_step_cancel(mocap);
                notify(QStringLiteral("校准状态版本不匹配，请使用本平台随包提供的库。"));
                return;
            }
        }
        sevenActive=true; sevenSaved=false; sevenFinished=false; sevenFailureNotified=false;
        for(int h=0;h<2;++h) if(mask&(1u<<h)) angles.reset(h);
        notify(QStringLiteral("已启动七步校准：%1")
            .arg(handNames(mask)));
    };
    QObject::connect(r, &QPushButton::clicked, [&] {
        if (sevenActive) { notify(QStringLiteral("请先完成或取消七步校准")); return; }
        if (quickRMask) return;
        outputTabs->setCurrentIndex(1);
        HexPartnerStatus current{}; current.struct_size = sizeof(current);
        hex_partner_get_status(partner, &current);
        const uint32_t mask = current.streaming_hand_mask & 3u;
        if (!mask || !hex_mocap_start_r_capture_remount(mocap, int(mask), quickRSeconds)) {
            finishQuickR(QStringLiteral("无法开始 R 快速校准，请等到手套输出有效数据后重试。"));
            return;
        }
        sevenFinished = false;
        quickRMask = mask;
        quickRDeadline = clock.elapsed() + quickRTimeoutMs;
        r->setEnabled(false);
        calibrationText->setText(QStringLiteral("R 快速校准 · %1 · 0%\n请平放手掌、自然伸直手指并保持不动。").arg(handNames(mask)));
        notify(QStringLiteral("已开始采集%1的佩戴零位").arg(handNames(mask)));
    });
    QObject::connect(full, &QPushButton::clicked, [&] { if(!sevenActive) beginSeven(); });
    QObject::connect(cancelR, &QPushButton::clicked, [&] {
        if (sevenActive) {
            if (!hex_mocap_seven_step_cancel(mocap)) { notify(QStringLiteral("七步校准取消失败，请勿保存")); return; }
            sevenActive=false;
            notify(QStringLiteral("七步校准已取消；本轮数据回滚。再次采集前请重新进行基准校准。"));
        } else {
            cancelQuickR(QStringLiteral("R 快速校准已取消。"));
        }
    });
    QObject::connect(retryR, &QPushButton::clicked, [&] {
        if (sevenActive) {
            if (!hex_mocap_seven_step_cancel(mocap)) { notify(QStringLiteral("无法回滚当前校准")); return; }
            sevenActive=false;
        }
        beginSeven();
    });
    QObject::connect(retryContact, &QPushButton::clicked, [&] {
        if(!sevenActive) return;
        HexMocapSevenStepState state{}; state.struct_size=sizeof(state);
        if(hex_mocap_seven_step_get_state(mocap,&state) && state.step>=4 && state.step<=7 &&
           hex_mocap_contact_retry(mocap,int(state.hand_mask))) {
            sevenFailureNotified=false;
            notify(QStringLiteral("已重试当前对指动作：%1").arg(handNames(state.hand_mask)));
        }
    });
#ifdef Q_OS_WIN
    QObject::connect(recapture, &QPushButton::clicked, &window, [&] {
        HexMocapSevenStepState state{};
        state.struct_size = sizeof(state);
        if (sevenActive && hex_mocap_seven_step_get_state(mocap, &state)
            && hex_mocap_contact_recapture(mocap, int(state.hand_mask))) {
            sevenFailureNotified = false;
            notify(QStringLiteral("请重新采集当前手指的捏合与掐合姿态。"));
        }
    });
#endif
    QObject::connect(saveProfile, &QPushButton::clicked, [&] {
        if (sevenActive || quickRMask) { notify(QStringLiteral("请等待校准完成后再保存")); return; }
        notify(hex_partner_profile_save(partner) ? "校准档案已保存" :
            errorName(HEX_PARTNER_PROFILE_SAVE_FAILED));
    });
    QObject::connect(loadProfile, &QPushButton::clicked, [&] {
        if (sevenActive || quickRMask) { notify(QStringLiteral("请先完成或取消校准，再加载档案")); return; }
        if(hex_partner_profile_load(partner)) {
            notify(QStringLiteral("校准档案已加载，下一帧将按双手掌面重新对齐观察方向"));
        } else notify(QStringLiteral("校准档案加载失败"));
    });
    QObject::connect(reconnect, &QPushButton::clicked, [&] {
        HexPartnerStatus current{}; current.struct_size = sizeof(current);
        if (partner) hex_partner_get_status(partner, &current);
        if (!partner || current.phase == HEX_PARTNER_STOPPED || current.phase == HEX_PARTNER_FAILED) {
            startConnection(selectedConnection());
        } else {
            cancelQuickR(QStringLiteral("连接已断开，R 快速校准已取消。"));
            if (sevenActive) { hex_mocap_seven_step_cancel(mocap); sevenActive = false; }
            hex_partner_disconnect(partner);
            canvas->resetViewBasis();
        }
        current = {};
        current.struct_size = sizeof(current);
        if (partner) hex_partner_get_status(partner, &current);
        updateConnectionControls(current);
    });
    QObject::connect(streamButton, &QPushButton::clicked, [&] {
        HexPartnerStatus current{}; current.struct_size=sizeof(current);
        if(!hex_partner_get_status(partner,&current)) return;
        if(sevenActive || quickRMask){notify(QStringLiteral("校准期间不能暂停数据流"));return;}
        if(!hex_partner_stream_control(partner,current.stream_requested?0:1))
            notify(QStringLiteral("数据流命令失败：%1").arg(errorName(HEX_PARTNER_STREAM_COMMAND_FAILED)));
        else notify(current.stream_requested?QStringLiteral("已发送暂停数据流命令"):QStringLiteral("已发送恢复数据流命令，等待新帧"));
    });
    auto toggleWristXY = [&] {
        if (quickRMask || sevenActive) { notify(QStringLiteral("请等待校准结束后再切换腕部同步")); return; }
        // Unity 的 F6 直接修改每只手 pipeline 的腕部节点 sync_axis_mask 低两位。
        // 保留其他配置位和所有模型参数，且两只手要同时修改成功。
        HexMocapPipelineConfig configs[2]{};
        bool allEnabled = true;
        for (int h = 0; h < 2; ++h) {
            configs[h].struct_size = sizeof(configs[h]);
            if (!hex_mocap_get_pipeline_config(mocap, h, &configs[h])) {
                notify(QStringLiteral("无法读取腕部配置，请检查 SDK 库")); return;
            }
            allEnabled &= (configs[h].model_driver.node[0].sync_axis_mask & 3u) == 3u;
        }
        const bool enable = !allEnabled;
        for (int h = 0; h < 2; ++h) {
            auto next = configs[h];
            auto& mask = next.model_driver.node[0].sync_axis_mask;
            mask = enable ? mask | 3u : mask & ~3u;
            if (!hex_mocap_set_pipeline_config(mocap, h, &next)) {
                for (int restored = 0; restored < h; ++restored)
                    hex_mocap_set_pipeline_config(mocap, restored, &configs[restored]);
                notify(QStringLiteral("腕部 XY 设置失败，已尝试恢复原配置")); return;
            }
        }
        if (enable) {
            // 从当前正面掌心视角开始显示后续腕部运动。
            canvas->resetViewBasis();
            for(int h=0;h<2;++h) canvas->captureViewBasis(h);
        }
        canvas->wristXyEnabled = enable;
        canvas->update();
        notify(enable ? QStringLiteral("腕部 XY 同步已开启") : QStringLiteral("腕部 XY 同步已锁定"));
    };
    auto* wristShortcutF5 = new QShortcut(QKeySequence(Qt::Key_F5), &window);
    auto* wristShortcutF6 = new QShortcut(QKeySequence(Qt::Key_F6), &window);
    QObject::connect(wristShortcutF5, &QShortcut::activated, toggleWristXY);
    QObject::connect(wristShortcutF6, &QShortcut::activated, toggleWristXY);
    auto* frontShortcutF8 = new QShortcut(QKeySequence(Qt::Key_F8), &window);
    QObject::connect(frontShortcutF8, &QShortcut::activated, [&] {
        canvas->resetViewBasis();
        for(int h=0;h<2;++h) canvas->captureViewBasis(h);
        canvas->update();
        notify(QStringLiteral("已按当前双手掌面重置正面视角（F8）"));
    });
    QObject::connect(angle_zero, &QPushButton::clicked, [&] {
        if (sevenActive) { notify(QStringLiteral("请先完成或取消七步校准")); return; }
        outputTabs->setCurrentIndex(0);
        // 用户确认手指伸直后，分别校准 Hub 与 finger[5][4] 精确角零位。
        if (quickRMask) { angle_text->setPlainText("请等待 R 快速校准完成。"); return; }
        int done = 0;
        for (int h = 0; h < 2; ++h) {
            if (!canvas->streaming[h] || !canvas->hasBaseline[h]) continue;
            HexMocapDisplayPose pose{};
            pose.struct_size = sizeof(pose);
            if (hex_mocap_get_display_pose(mocap, h, &pose) && angles.calibrate(h, pose)) {
                hex_mocap_calibrate_angles(mocap, 1 << h);
                angleSampleReady[h] = false;
                ++done;
            }
        }
        showingAngleSamples = false;
        angle_text->setPlainText(done ? QString("已完成 %1 只手的角度归零。").arg(done)
                                 : "请先完成 R 快速校准并确认手部正在跟踪。");
    });
    qint64 nextTextUpdate = 0;
    qint64 nextUdpPreview = 0;
    uint32_t previousPhase = UINT32_MAX;
    uint32_t previousStreamingMask = 0;
    QByteArray udpPayload(int(udpBufferSize), '\0');
    QObject::connect(outputTabs, &QTabWidget::currentChanged, &window, [&](int) {
        nextTextUpdate = 0;
        nextUdpPreview = 0;
    });
    QTimer renderTimer;
    QObject::connect(&renderTimer, &QTimer::timeout, &window, [&] {
        if (canvasDirty) {
            canvasDirty = false;
            canvas->update();
        }
    });
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&] {
        if (!partner) return;
        // 设备采样保持独立节奏；文字和画布按显示频率刷新。
        const qint64 now = clock.elapsed();
        HexPartnerStatus current{}; current.struct_size = sizeof(current);
        hex_partner_tick(partner, now);
        hex_partner_get_status(partner, &current);
        const bool phaseChanged = current.phase != previousPhase;
        const bool streamingChanged = current.streaming_hand_mask != previousStreamingMask;
        const bool refreshText = phaseChanged || streamingChanged || now >= nextTextUpdate;
        const bool refreshCalibration = refreshText && outputTabs->currentWidget() == calibrationScroll;
        const bool refreshAngles = refreshText && outputTabs->currentWidget() == angle_text;
        if (refreshText) nextTextUpdate = now + textIntervalMs;
        if (phaseChanged && (current.phase == HEX_PARTNER_STOPPED || current.phase == HEX_PARTNER_FAILED)) {
            canvas->pose[0]={}; canvas->pose[1]={};
            canvas->resetViewBasis();
            canvasDirty = true;
        }
        if (phaseChanged || streamingChanged) {
            if (!current.streaming_hand_mask) {
                angle_text->setPlainText(QStringLiteral("当前没有实时手部数据，请检查连接。"));
                udpText->setPlainText(QStringLiteral("数据输出已暂停，等待手套恢复连接。"));
                showingAngleSamples = false;
            } else if (!previousStreamingMask) {
                angle_text->setPlainText(angleInstructions);
                udpText->setPlainText(udpWaitingText);
                nextUdpPreview = 0;
            }
        }
        previousPhase = current.phase;
        previousStreamingMask = current.streaming_hand_mask;
        if (quickRMask) {
            HexMocapRCaptureInfo capture{};
            capture.struct_size = sizeof(capture);
            if (current.phase == HEX_PARTNER_STOPPED || current.phase == HEX_PARTNER_RETRYING ||
                current.phase == HEX_PARTNER_FAILED) {
                cancelQuickR(QStringLiteral("连接已中断，本次 R 快速校准已取消。"));
            } else if (!hex_mocap_get_r_capture_info(mocap, &capture)) {
                cancelQuickR(QStringLiteral("无法读取 R 快速校准进度，请重试。"));
            } else if (capture.state == HEX_MOCAP_R_COMPLETE && capture.hand_mask == int(quickRMask)) {
                for (int hand = 0; hand < 2; ++hand) {
                    if (!(quickRMask & (1u << hand))) continue;
                    angles.reset(hand);
                    angleSampleReady[hand] = false;
                    canvas->viewBasisValid[hand] = false;
                }
                canvasDirty = true;
                showingAngleSamples = false;
                angle_text->setPlainText(QStringLiteral("佩戴零位已更新。查看实时角度前，请伸直手指并点击「角度归零」。"));
                if (noProfile) {
                    finishQuickR(QStringLiteral("R 快速校准完成，新的手掌和手指零位已在本次会话生效。"));
                } else if (hex_partner_profile_save(partner)) {
                    finishQuickR(QStringLiteral("R 快速校准完成，手掌和手指的佩戴零位已更新并保存到当前串口档案。"));
                } else {
                    finishQuickR(QStringLiteral("R 快速校准完成，佩戴零位已更新。自动保存失败，可手动保存档案。"));
                }
            } else if (capture.state != HEX_MOCAP_R_SAMPLING || now >= quickRDeadline) {
                cancelQuickR(QStringLiteral("R 快速校准未完成。请检查手套连接，平放手掌后重试。"));
            } else if (refreshCalibration) {
                const auto progressText = QStringLiteral("R 快速校准 · %1 · %2%\n请平放手掌、自然伸直手指并保持不动。")
                    .arg(handNames(quickRMask)).arg(capture.progress * 100.f, 0, 'f', 0);
                if (calibrationText->text() != progressText) calibrationText->setText(progressText);
            }
        }
        if (refreshText) {
            HexMocapPipelineConfig wristConfig{};
            wristConfig.struct_size = sizeof(wristConfig);
            if (hex_mocap_get_pipeline_config(mocap, 0, &wristConfig)) {
                const bool enabled = (wristConfig.model_driver.node[0].sync_axis_mask & 3u) == 3u;
                if (canvas->wristXyEnabled != enabled) {
                    canvas->wristXyEnabled = enabled;
                    canvasDirty = true;
                }
            }
        }
        if(sevenActive && (current.phase==HEX_PARTNER_STOPPED || current.phase==HEX_PARTNER_RETRYING || current.phase==HEX_PARTNER_FAILED)) {
            hex_mocap_seven_step_cancel(mocap);
            sevenActive=false;
            notify(QStringLiteral("连接已重建或断开，本轮七步校准未完成，请重新开始"));
        }
        if (sevenActive) {
            HexMocapSevenStepState seven{}; seven.struct_size=sizeof(seven);
            if (!hex_mocap_seven_step_get_state(mocap,&seven)) {
                if (refreshCalibration) calibrationText->setText(QStringLiteral("无法读取七步校准状态，请检查 SDK 库"));
            } else {
#ifdef Q_OS_WIN
                recapture->setEnabled(seven.state == 4 && seven.step >= 4 && seven.step <= 7);
#endif
                if (refreshCalibration) {
                    const int step=qBound(1,int(seven.step),7);
                    QString prompt;
                    if(step==1) prompt=QStringLiteral("%1手肘放于桌面，五指自然放松伸直并保持不动。").arg(handNames(seven.hand_mask));
                    else if(step==2) prompt=QStringLiteral("四指伸直，拇指靠在手侧，保持不动。");
                    else if(step==3) prompt=QStringLiteral("四指握拳，拇指伸直并尽量向外张开，保持不动。");
                    else prompt=
#ifdef Q_OS_WIN
                        QStringLiteral("拇指与%1先保持指腹捏合。看到已保存后，可松开，再保持指尖掐合。")
#else
                        QStringLiteral("拇指与%1保持接触，慢慢从指腹捏到指尖掐，再放松回捏。")
#endif
                        .arg(QStringList{QStringLiteral("食指"),QStringLiteral("中指"),QStringLiteral("无名指"),QStringLiteral("小指")}[step-4]);
                    QStringList handProgress;
                    float total=0.f; int count=0;
                    for(int h=0;h<2;++h) if(seven.hand_mask&(1u<<h)) {
#ifdef Q_OS_WIN
                        if (step >= 4) {
                            HexMocapContactEndpointStatus endpoint{};
                            endpoint.struct_size = sizeof(endpoint);
                            if (hex_mocap_get_contact_endpoint_status(mocap, h, &endpoint)) {
                                const float p = qBound(0.f, endpoint.phase_progress, 1.f);
                                float progress = (endpoint.completed_mask & 1u) ? 0.5f : 0.f;
                                if ((endpoint.completed_mask & 3u) == 3u) progress = 1.f;
                                else if (endpoint.phase == HEX_MOCAP_ENDPOINT_PINCH) progress = 0.5f * p;
                                else if (endpoint.phase == HEX_MOCAP_ENDPOINT_CLAW) progress = 0.5f + 0.5f * p;
                                total += progress;
                                ++count;
                                handProgress << QStringLiteral("%1：%2（%3%）").arg(handNames(1u << h))
                                    .arg(endpointPrompt(endpoint)).arg(progress * 100.f, 0, 'f', 0);
                                continue;
                            }
                            ++count;
                            handProgress << QStringLiteral("%1：等待两姿态状态").arg(handNames(1u << h));
                            continue;
                        }
#endif
                        HexMocapContactCaptureStatus capture{}; capture.struct_size=sizeof(capture);
                        const bool hasContact=step>=4 && hex_mocap_get_contact_capture_status(mocap,h,&capture);
                        const float progress=hasContact?capture.progress:seven.progress;
                        total+=progress; ++count;
                        handProgress << QStringLiteral("%1：%2% %3").arg(h?QStringLiteral("右手"):QStringLiteral("左手"))
                            .arg(progress*100.f,0,'f',0)
                            .arg(hasContact?QStringLiteral("捏→掐采样 %1 帧").arg(capture.total_samples):QString());
                    }
                    const float progress=count?total/float(count):seven.progress;
                    const auto progressText = QStringLiteral("Unity 七步校准 · %1（%2/7）\n总进度：%3%\n%4\n\n%5\n\n状态：%6")
                        .arg(sevenTitles[step-1]).arg(step).arg(progress*100.f,0,'f',0)
                        .arg(handProgress.join("\n")).arg(prompt)
                        .arg(seven.state==2?QStringLiteral("完成"):seven.state==4?QStringLiteral("采样不足，请重新校准"):QStringLiteral("自动采集中"));
                    if (calibrationText->text() != progressText) calibrationText->setText(progressText);
                }
                if(seven.state==2) {
                    sevenActive=false;
                    sevenFinished=true;
                    if(noProfile) notify(QStringLiteral("七步校准完成，本次会话已生效。"));
                    else if(!sevenSaved){sevenSaved=hex_partner_profile_save(partner)!=0;
                        notify(sevenSaved?QStringLiteral("七步校准完成，配置已自动保存"):QStringLiteral("七步校准完成，但配置保存失败，请手动保存"));}
                } else if(seven.state==4 && !sevenFailureNotified) {
                    sevenFailureNotified=true;
                    notify(QStringLiteral("七步校准失败或采样不足，请重新校准"));
                }
            }
        } else if(sevenFinished && refreshCalibration) {
            const auto completion = noProfile ? QStringLiteral("七步校准已完成，本次会话已生效。")
                : sevenSaved ? QStringLiteral("七步校准已完成，当前配置已保存。")
                : QStringLiteral("Unity 七步校准已完成，但配置保存失败，请点击左侧保存校准档案。");
            if (calibrationText->text() != completion) calibrationText->setText(completion);
        }
        HexPartnerEvent event{}; event.struct_size = sizeof(event);
        while (hex_partner_get_event(partner, eventSequence, &event) == 1) {
            eventSequence = event.sequence;
            if (event.kind == HEX_PARTNER_EVENT_PROFILE_LOADED) {
                usesUnityDefaults = false;
                profileSource->setText(QStringLiteral("当前档案：%1").arg(activeConnection.description()));
                refreshPreviews();
                for (int hand = 0; hand < 2; ++hand) angles.reset(hand);
            }
            notify(QStringLiteral("%1 · 操作 %2 · %3")
                .arg(eventName(event.kind)).arg(event.operation_sequence)
                .arg(event.error ? errorName(event.error) : QStringLiteral("成功")));
        }
        // 与原 Unity 广播字段保持一致：0 角度、1 姿态、2 位置、3 旋转。
        // 此处只预览编码结果；网络发送需由应用自行实现。
        if (current.fresh_hand_mask && outputTabs->currentWidget() == udpText && now >= nextUdpPreview) {
            nextUdpPreview = now + textIntervalMs;
            bool preciseAnglesReady = !(current.fresh_hand_mask & quickRMask);
            for (int hand = 0; hand < 2; ++hand) {
                if (!(current.fresh_hand_mask & (1u << hand))) continue;
                HexAnglesSnapshot snapshot{};
                snapshot.struct_size = sizeof(snapshot);
                if (!hex_mocap_get_precise_angles(mocap, hand, &snapshot)) {
                    preciseAnglesReady = false;
                    break;
                }
            }
            const auto utc = QDateTime::currentMSecsSinceEpoch();
            QStringList channels;
            for (uint32_t channel = 0; channel < 4; ++channel) {
                if (channel == 0 && !preciseAnglesReady) {
                    channels << (quickRMask ? QStringLiteral("通道 0（角度）：正在进行 R 快速校准。")
                                            : QStringLiteral("通道 0（角度）：等待角度校准。"));
                    continue;
                }
                const uint32_t needed = hex_partner_encode_legacy_udp(partner, channel, utc,
                    udpPayload.data(), udpBufferSize);
                if (needed && needed <= udpBufferSize) {
                    channels << QStringLiteral("通道 %1：%2").arg(channel)
                        .arg(QString::fromUtf8(udpPayload.constData(), int(needed - 1)));
                } else {
                    channels << QStringLiteral("通道 %1：协议数据编码失败。").arg(channel);
                }
            }
            udpText->setPlainText(channels.join("\n\n"));
        }
        QStringList angle_lines;
        for (int hand = 0; hand < 2; ++hand) {
            auto& p = canvas->pose[hand];
            const uint32_t bit = 1u << hand;
            const bool streaming = (current.streaming_hand_mask & bit) != 0;
            HexMocapHandDriverState driver{};
            driver.struct_size = sizeof(driver);
            hex_mocap_get_hand_driver_state(mocap, hand, &driver);
            const bool hasBaseline = (driver.flags & HEX_MOCAP_HAND_HAS_R) != 0;
            if (streaming != canvas->streaming[hand] || hasBaseline != canvas->hasBaseline[hand]) {
                canvas->streaming[hand] = streaming;
                canvas->hasBaseline[hand] = hasBaseline;
                canvasDirty = true;
            }
            if (usesUnityDefaults && !quickRMask && !sevenActive && (current.fresh_hand_mask & bit) &&
                (driver.layout == 7 || driver.layout == 11) && driver.layout != inputLayouts[hand] &&
                applyUnityInputLayout(mocap, hand, driver.layout)) {
                inputLayouts[hand] = driver.layout;
                canvas->hasBaseline[hand] = false;
                canvas->viewBasisValid[hand] = false;
                p = {};
                angles.reset(hand);
                canvasDirty = true;
                notify(QStringLiteral("%1已切换到 Unity M%2 源骨架，请重新进行 R 快速校准。")
                    .arg(handNames(bit)).arg(driver.layout));
            }
            if (canvas->hasBaseline[hand] && (current.fresh_hand_mask & bit)) {
                HexMocapDisplayPose fresh{}; fresh.struct_size = sizeof(fresh);
                if (hex_mocap_get_display_pose(mocap, hand, &fresh)) {
                    p = fresh;
                    canvasDirty = true;
                }
            }
            if (canvasDirty) canvas->captureViewBasis(hand);
            // 导出仍按 SDK 新帧进行，界面只格式化当前显示的角度。
            if (!streaming || !canvas->hasBaseline[hand] || !angles.calibrated[hand])
                angleSampleReady[hand] = false;
            if (canvas->hasBaseline[hand] && !(quickRMask & bit) && angles.update(hand, p))
                angleSampleReady[hand] = true;
            if (refreshAngles && streaming && !(quickRMask & bit) && angleSampleReady[hand]) {
                const auto& f = angles.state[hand].flexion;
                angle_lines << QString("%1 · 时间 %2 毫秒\n拇指：掌指关节 %3°，指间关节 %4°\n食指：%5° / %6° / %7°　中指：%8° / %9° / %10°\n无名指：%11° / %12° / %13°　小指：%14° / %15° / %16°")
                    .arg(hand ? "右手" : "左手").arg(angles.last_timestamp[hand])
                    .arg(f[2], 0, 'f', 1).arg(f[3], 0, 'f', 1)
                    .arg(f[5], 0, 'f', 1).arg(f[6], 0, 'f', 1).arg(f[7], 0, 'f', 1)
                    .arg(f[9], 0, 'f', 1).arg(f[10], 0, 'f', 1).arg(f[11], 0, 'f', 1)
                    .arg(f[13], 0, 'f', 1).arg(f[14], 0, 'f', 1).arg(f[15], 0, 'f', 1)
                    .arg(f[17], 0, 'f', 1).arg(f[18], 0, 'f', 1).arg(f[19], 0, 'f', 1);
                HexAnglesSnapshot precise{};
                precise.struct_size = sizeof(precise);
                if (hex_mocap_get_precise_angles(mocap, hand, &precise) && precise.calibrated)
                    // 精确角时间戳来自快照；拇指 CMC 使用 Hub 槽 1。
                    angle_lines << QString("%1精确角 · 时间 %2 毫秒：拇指掌指关节 %3°，食指掌指关节 %4°")
                        .arg(hand ? "右手" : "左手").arg(precise.timestamp_ms)
                        .arg(precise.finger[0][0], 0, 'f', 1).arg(precise.finger[1][0], 0, 'f', 1);
            }
        }
        if (refreshAngles) {
            if (!angle_lines.isEmpty()) {
                const auto angleDisplay = angle_lines.join("\n");
                if (angle_text->toPlainText() != angleDisplay) angle_text->setPlainText(angleDisplay);
                showingAngleSamples = true;
            } else if (showingAngleSamples) {
                angle_text->setPlainText(angleInstructions);
                showingAngleSamples = false;
            }
        }
        if (!refreshText) return;
        updateConnectionControls(current);
        streamButton->setText(current.stream_requested?QStringLiteral("暂停数据流"):QStringLiteral("恢复数据流"));
        HexGloveDeviceSnapshot d{};
        hex_glove_get_device_snapshot(glove, &d);
        HexPartnerTiming timing{};
        timing.struct_size = sizeof(timing);
        hex_partner_get_timing(partner, &timing);
        const uint32_t statusError = current.operation_error ? current.operation_error : current.last_error;
        QString connectionText = current.phase == HEX_PARTNER_STOPPED
            ? QStringLiteral("未连接") : activeConnection.description();
        if (current.phase != HEX_PARTNER_STOPPED && !activeConnection.wired) {
            char receiverId[7]{};
            const QString id = hex_glove_get_device_id(glove, receiverId, sizeof(receiverId))
                ? QString::fromLatin1(receiverId) : QStringLiteral("等待设备回复");
            connectionText += QStringLiteral("　｜　接收器设备 ID：%1").arg(id);
        }
        const auto statusText = QStringLiteral("%1　｜　%2　｜　输出：%3　｜　新帧：%4\n"
            "左手：%5 帧/秒 · 电量 %6%　　右手：%7 帧/秒 · 电量 %8%　　双手到达间隔：%9 毫秒\n"
            "操作 %10：%11　｜　错误：%12　｜　串口阶段/系统码：%13/%14　｜　腕部：%15")
            .arg(connectionText).arg(phaseName(current.phase))
            .arg(handNames(current.streaming_hand_mask)).arg(handNames(current.fresh_hand_mask))
            .arg(timing.observed_hz[0], 0, 'f', 1).arg(d.batt_left, 0, 'f', 0)
            .arg(timing.observed_hz[1], 0, 'f', 1).arg(d.batt_right, 0, 'f', 0)
            .arg(timing.hand_arrival_gap_ms).arg(current.operation_sequence)
            .arg(operationName(current.operation_state)).arg(errorName(statusError))
            .arg(current.port_error_stage).arg(current.platform_error_code)
            .arg(canvas->wristXyEnabled?QStringLiteral("XY 开启"):QStringLiteral("XY 锁定"));
        if (status->text() != statusText) status->setText(statusText);
    });
    if ((initialConnection.wired || !initialConnection.receiver.isEmpty()) && !startConnection(initialConnection))
        return 5;
    HexPartnerStatus initialStatus{};
    initialStatus.struct_size = sizeof(initialStatus);
    if (partner) hex_partner_get_status(partner, &initialStatus);
    updateConnectionControls(initialStatus);
    if (!partner) status->setText(QStringLiteral("请选择连接方式和串口，然后点击「连接手套」。"));
    // observed_hz 按本应用实际取得的新帧计算；33 ms 轮询会人为限制到约 30 Hz。
    timer.setTimerType(Qt::PreciseTimer);
    timer.start(5);
    renderTimer.setTimerType(Qt::PreciseTimer);
    renderTimer.start(renderIntervalMs);
    window.setWindowTitle("HexacercleGlove");
    window.show();
    const int result = app.exec();
    // 先停止更新，再断开设备连接。
    renderTimer.stop();
    timer.stop();
    if (quickRMask) hex_mocap_cancel_r(mocap);
    if (partner) hex_partner_disconnect(partner);
    return result;
}
