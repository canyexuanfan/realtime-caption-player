// src/player/MainWindow.cpp
// 逐像素复刻 05_Single_HTML_Frontend_Reference.html（除实例数据外全部一致）：
// 设计令牌、布局尺寸、控件样式、设置面板 7 页均对照参考 HTML。
// 数据全部真实：播放列表/历史/转写/统计来自实际播放与 worker 事件；
// 引擎信息为实际双引擎；无后端的能力不显示可假用的控件。
#include "MainWindow.h"
#include "MpvPlayer.h"
#include "MpvRenderWidget.h"
#include "MpvTrace.h"
#include "UiKit.h"
#include "WorkerSupervisor.h"
#include "captions/CaptionController.h"
#include "captions/SrtExporter.h"
#include "captions/CaptionTypes.h"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QMimeData>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QShortcut>
#include <QSlider>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QStandardPaths>
#include <QStandardItemModel>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

using rcpui::CaptionLabel;
using rcpui::Switch;
using rcpui::Waveform;
using rcpui::svgIcon;

namespace {
const QColor kMuted("#a7adb8"), kDim("#757c88"),
    kSuccess("#61d27d"), kWarning("#f2bc58"),
    kDanger("#ff6b79"), kSpeedPurple("#9c8fff"), kIcon("#c9cedb");

QIcon icon(const QString& name, const QColor& c = kIcon, int px = 17) {
    return svgIcon(name, c, px);
}

// 参考设计 :root 令牌 → QSS
const char* kAppQss = R"(
* { outline: none; }
QWidget { background: #090b0f; color: #f4f6fb; font-family: "Segoe UI","Microsoft YaHei UI"; font-size: 14px; }

QWidget#titleBar { background: #111419; border-bottom: 1px solid rgba(255,255,255,0.105); }
QLabel#brand { color:#f4f6fb; font-size:13px; font-weight:650; background:transparent; }
QLabel#titleFile { color:#f4f6fb; font-size:16px; font-weight:500; background:transparent; }

QFrame#leftRail { background:#15191f; border:none; border-right:1px solid rgba(255,255,255,0.105); }
QLabel[class="railTitle"] { color:#f4f6fb; font-size:12px; font-weight:600; background:transparent; }
QPushButton[class="iconBtn"] { background:transparent; border:none; border-radius:7px; color:#a7adb8; padding:0; }
QPushButton[class="iconBtn"]:hover { background:#222832; color:#f4f6fb; }
QListWidget { background:#15191f; border:none; outline:none; }
QListWidget#playlist, QListWidget#history { padding:0 9px 8px 9px; }
QListWidget#playlist::item, QListWidget#history::item { min-height:40px; }
QListWidget::item { color:#a7adb8; border-radius:5px; padding:4px 9px; margin:0 0 1px 0; }
QListWidget::item:hover { background:#222832; color:#f4f6fb; }
QListWidget::item:selected { background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #5147aa, stop:1 #6254cb); color:#fff; }
QListWidget#transcript { padding:0 9px 10px 9px; }
QLabel#transcriptItem { background:transparent; color:#a7adb8; font-size:12px; }
QLineEdit#searchBox {
  background:#20252d; border:1px solid rgba(255,255,255,0.105); border-radius:6px;
  color:#f4f6fb; font-size:12px; min-height:31px; padding:0 8px; selection-background-color:#5c4bd5;
}
QLabel#railStats { color:#757c88; font-size:11px; background:transparent; }

QFrame#videoSurface { background:#10151d; border:none; }
QLabel#videoFileTitle { color:rgba(255,255,255,0.92); font-size:17px; font-weight:500; background:transparent; }
QWidget#privacyBadge { background:rgba(10,13,18,0.46); border:1px solid rgba(255,255,255,0.13); border-radius:14px; }
QWidget#privacyBadge QLabel { background:transparent; color:rgba(255,255,255,0.88); font-size:11px; }
QFrame#asrCard { background:rgba(16,19,24,0.86); border:1px solid rgba(255,255,255,0.16); border-radius:12px; }
QFrame#asrCard QLabel { background:transparent; }
QLabel#asrHead { color:#f5f6fa; font-size:12px; font-weight:600; }
QLabel#asrStatus { color:#d9dde4; font-size:11px; font-weight:400; }
QLabel[class="asrKey"] { color:#9da4af; font-size:10px; background:transparent; }
QLabel#selectShell { color:#e8eaf0; font-size:10px; background:rgba(255,255,255,0.025);
  border:1px solid rgba(255,255,255,0.13); border-radius:5px; padding:0 8px; }
QLabel#asrMetrics { color:#aeb4bd; font-size:10px; background:transparent; }
QLabel#asrMetricVal { color:#e4e6eb; font-size:10px; font-weight:500; background:transparent; }
QPushButton#asrExport { background:transparent; border:1px solid rgba(255,255,255,0.14);
  border-radius:5px; color:#9e91ff; min-height:30px; font-size:10px; }
QPushButton#asrExport:hover { background:rgba(121,105,255,0.13); }

QWidget#controlDeck { background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #15191e, stop:1 #111419);
  border:none; border-top:1px solid rgba(255,255,255,0.05); }
QWidget#controlDeck QLabel { background:transparent; color:#f5f6fa; font-size:11px; }
QPushButton#btnSpeedText { background:transparent; border:none; border-radius:6px; color:#9c8fff;
  font-size:12px; min-height:34px; padding:0 5px; }
QPushButton#btnSpeedText:hover { background:rgba(255,255,255,0.07); }

QSlider { min-height:16px; }
QSlider::groove:horizontal { border:none; height:4px; border-radius:2px; background:rgba(255,255,255,0.16); }
QSlider::sub-page:horizontal { background:#7868ff; border-radius:2px; }
QSlider::handle:horizontal { width:12px; height:12px; margin:-4px 0; border-radius:6px; background:#7e6cff; }
QSlider::handle:horizontal:hover { background:#9184ff; }

QFrame#settingsPanel { background:#15191f; border:none; border-left:1px solid rgba(255,255,255,0.105); }
QLabel#settingsTitle { color:#f4f6fb; font-size:13px; font-weight:600; background:transparent; }
QLabel[class="paneTitle"] { color:#f4f6fb; font-size:13px; font-weight:700; background:transparent; }
QLabel[class="secTitle"] { color:#f4f6fb; font-size:11px; font-weight:650; background:transparent; }
QLabel[class="rowLabel"] { color:#a7adb8; font-size:10px; background:transparent; }
QLabel[class="rowHelp"] { color:#757c88; font-size:9px; background:transparent; }
QComboBox[class="fieldSelect"] { background:#171b21; border:1px solid rgba(255,255,255,0.16);
  border-radius:5px; color:#f4f6fb; font-size:10px; min-height:28px; padding:0 9px; }
QComboBox QAbstractItemView { background:#191e25; color:#f4f6fb; selection-background-color:#5c4bd5; }

QScrollBar:vertical { background:transparent; width:6px; margin:0; }
QScrollBar::handle:vertical { background:rgba(255,255,255,0.18); border-radius:3px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:rgba(255,255,255,0.28); }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background:transparent; }
)";
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_player = new MpvPlayer(this);
    if (m_player->create()) {
        m_player->setOption(QStringLiteral("vo"), QStringLiteral("libmpv"));
        m_player->setOption(QStringLiteral("hwdec"),
                            m_settings.value(QStringLiteral("playback/hwdec"), QStringLiteral("no")).toString());
        // Intel 驱动兼容：高级渲染管线建视频纹理报 INVALID_ENUM（纹理创建失败
        // → 帧永远不显示 → 黑屏，播放时钟正常）。dumb 模式用固定 rgba8 管线绕开。
        m_player->setOption(QStringLiteral("gpu-dumb-mode"), QStringLiteral("yes"));
        m_player->setOption(QStringLiteral("screenshot-format"), QStringLiteral("png"));
        m_player->setOption(QStringLiteral("screenshot-directory"),
                            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        // 产品字幕只走本地 ASR 叠加层：禁用 mpv 外挂字幕自动加载与默认字幕轨。
        // 否则媒体同目录的历史导出 .srt 会被 mpv 渲染成第二层大字
        // （与 ASR 叠加重影，用户见"字幕重复/样式不对"）。手动加载走
        // loadSubtitle（sub-add select 立即显示）。
        m_player->setOption(QStringLiteral("sub-auto"), QStringLiteral("no"));
        m_player->setOption(QStringLiteral("sid"), QStringLiteral("no"));
        m_player->initialize();
    }

    m_captionCtl = new rcp::captions::CaptionController(this);
    m_worker = new rcp::player::WorkerSupervisor(this);
    rcpMark("ctor:player-initialized");

    rcpMark("ctor:applyTheme-begin");
    applyTheme();
    rcpMark("ctor:applyTheme-done");

    connect(m_player, &MpvPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &MpvPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player, &MpvPlayer::mediaLoaded, this, &MainWindow::onMediaLoaded);
    connect(m_player, &MpvPlayer::mediaEnded, this, &MainWindow::onMediaEnded);
    connect(m_player, &MpvPlayer::mediaError, this, [this](const QString& msg) {
        // 打开/读取失败（文件不存在、挂载盘离线等）：显式提示，不再无声无息。
        appendTranscriptFinal(-1, tr("[打开失败] %1").arg(msg));
        setAsrStatus(tr("打开失败"), QStringLiteral("#ff6b79"));
    });
    connect(m_player, &MpvPlayer::pauseStateChanged, this, [this](bool paused) {
        m_playBtn->setIcon(icon(paused ? QStringLiteral("play") : QStringLiteral("pause"),
                                QColor("#ffffff"), 22));
    });

    connect(m_worker, &rcp::player::WorkerSupervisor::sessionStarted,
            m_captionCtl, &rcp::captions::CaptionController::reset);
    connect(m_worker, &rcp::player::WorkerSupervisor::sessionStarted, this, [this] {
        setAsrStatus(tr("识别中"), kWarning.name());
        m_finalCount = 0;
        m_finals.clear();
        m_partialText.clear();
        m_overlayFinalText.clear();
        m_lastFinalEndMs = 0;
        m_statLines->setText(tr("字幕行数：0"));
    });
    connect(m_worker, &rcp::player::WorkerSupervisor::captionSegment,
            this, [this](const rcp::CaptionSegment& seg, bool isPartial) {
                if (isPartial) {
                    m_captionCtl->onPartial(seg, seg.generation);
                    m_partialText = seg.text;
                    appendTranscriptPartial(seg.text);
                    m_waveform->pulse(1.0);
                    updateOverlay();   // partial 也要实时上叠加层（原缺失，仅 final 刷新）
                } else {
                    m_captionCtl->onFinal(seg, seg.generation);
                    m_finals.append(seg);
                    m_partialText.clear();
                    m_overlayFinalText = seg.text;   // 定稿常驻，直到下一句替换
                    if (seg.endMs > m_lastFinalEndMs) m_lastFinalEndMs = seg.endMs;
                    appendTranscriptFinal(seg.startMs, seg.text);
                    updateOverlay();
                }
            });
    connect(m_worker, &rcp::player::WorkerSupervisor::ready, this,
            [this] { setAsrStatus(tr("运行中"), kSuccess.name()); });
    connect(m_worker, &rcp::player::WorkerSupervisor::workerError, this,
            [this](const QString&) { setAsrStatus(tr("字幕错误"), kDanger.name()); });
    connect(m_worker, &rcp::player::WorkerSupervisor::workerFinished, this, [this] {
        setAsrStatus(tr("识别进程退出"), QColor("#69717d").name());
    });
    connect(m_player, &MpvPlayer::positionChanged, this, [this](double s) {
        if (qEnvironmentVariableIsEmpty("RCP_NO_OVERLAY"))   // 诊断开关：跳过字幕对齐/叠加
            m_captionCtl->setPlayheadMs(static_cast<long long>(s * 1000.0) + m_delayMs);
        if (!m_videoRectValid) repositionOverlays();   // video-params 就绪后补一次画面锚定
        updateOverlay();   // 播放头推进时刷新定稿选择（与语音同步显示）
    });

    setAcceptDrops(true);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowIcon(QIcon(QStringLiteral(":/logo.png")));

    // GUI 线程心跳：500ms 一跳。trace 心跳断流 = GUI 线程被阻塞的时间窗。
    {
        auto* hb = new QTimer(this);
        int* n = new int(0);
        connect(hb, &QTimer::timeout, this, [n] { rcpTrace(QStringLiteral("hb %1").arg(++*n)); });
        hb->start(500);
    }


    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    rcpMark("ctor:buildTitleBar-begin");
    buildTitleBar(central);
    rcpMark("ctor:buildTitleBar-done");
    root->addWidget(m_titleBar);

    auto* workspace = new QWidget(central);
    auto* wh = new QHBoxLayout(workspace);
    wh->setContentsMargins(0, 0, 0, 0);
    wh->setSpacing(0);
    wh->addWidget(buildLeftRail(workspace));
    wh->addWidget(buildStage(workspace), 1);
    m_settingsPanel = buildSettingsPanel(workspace);
    // 参考稿默认展开设置面板且停驻「实时字幕」页
    m_settingsPanel->setVisible(true);
    m_panes->setCurrentIndex(3);
    for (auto& [btn, i] : m_navBtns) btn->setChecked(i == 3);
    wh->addWidget(m_settingsPanel);
    root->addWidget(workspace, 1);

    rcpMark("ctor:setCentralWidget");
    setCentralWidget(central);
    m_titleBar->installEventFilter(this);
    rcpMark("ctor:loadHistory-begin");
    loadHistory();
    rcpMark("ctor:loadHistory-done");
    resize(1280, 800);
    setMinimumSize(1024, 660);

    // 快捷键
    auto sc = [this](const char* key, auto slot) {
        auto* s = new QShortcut(QKeySequence(QLatin1String(key)), this);
        connect(s, &QShortcut::activated, this, slot);
    };
    sc("Space", &MainWindow::onPlayPause);
    sc("Left",  [this] { m_player->seek(qBound(0.0, m_player->timePosition() - 5.0, m_duration), false); });
    sc("Right", [this] { m_player->seek(qBound(0.0, m_player->timePosition() + 5.0, m_duration), false); });
    sc("Up",    [this] { m_volume->setValue(qMin(100, m_volume->value() + 5)); });
    sc("Down",  [this] { m_volume->setValue(qMax(0, m_volume->value() - 5)); });
    sc("F", &MainWindow::onToggleFullscreen);
    sc("S", &MainWindow::onToggleSettings);
    sc("R", &MainWindow::onToggleCaption);
    sc("Ctrl+O", &MainWindow::onOpen);

    m_volume->setValue(m_settings.value(QStringLiteral("playback/volume"), 100).toInt());
    const double sp = m_settings.value(QStringLiteral("playback/speed"), 1.0).toDouble();
    QTimer::singleShot(0, this, [this, sp] { onSpeed(sp); });
    applyCaptionStyle();
    if (m_settings.value(QStringLiteral("general/resumeSession"), true).toBool()) {
        const QString last = m_settings.value(QStringLiteral("history/paths")).toStringList().value(0);
        if (!last.isEmpty())
            // 存在性交给播放器：同步 exists() 会阻塞在离线网盘挂载路径（实测 20s 卡死）。
            // 若启动媒体（argv/拖放）已打开则跳过恢复，避免网盘 loadfile 顶掉显式媒体。
            QTimer::singleShot(1600, this, [this, last] {
                rcpTrace(QStringLiteral("open-timer[resume] fire cur=%1 argv=%2")
                             .arg(m_currentPath.isEmpty() ? QStringLiteral("<empty>") : m_currentPath,
                                  m_startupMedia.isEmpty() ? QStringLiteral("<none>") : QStringLiteral("<set>")));
                // 命令行/拖放显式媒体优先：恢复会话直接让位，避免两个 loadfile 抢跑
                // 造成 replace 重载（end-file reason=2 → 停止/重启风暴）。
                // RCP_DEMO_ONLY=1：保持参考稿示例数据展示态（像素对比用）。
                if (!m_startupMedia.isEmpty() || !m_currentPath.isEmpty() ||
                    !qEnvironmentVariableIsEmpty("RCP_DEMO_ONLY")) return;
                openFile(last);
            });
    }

    // 参考稿复刻：断点响应 + 示例数据展示态（打开真实媒体即整体替换）。
    applyBreakpoint(width());
    fillDemoData();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    applyBreakpoint(width());
    repositionOverlays();
}

// @media (max-width:1320px)：--left-w 238→218、--settings-w 344→322、
// 标题动作钮 .label 隐藏（只留图标）。逐条对照参考 CSS。
void MainWindow::applyBreakpoint(int w) {
    const bool narrow = w <= 1320;
    if (m_leftRail) m_leftRail->setFixedWidth(narrow ? 218 : 238);
    if (m_settingsPanel) m_settingsPanel->setFixedWidth(narrow ? 322 : 344);
    for (int i = 0; i < m_titleActions.size(); ++i) {
        if (!m_titleActions.at(i)) continue;
        m_titleActions.at(i)->setText(narrow ? QString() : m_titleActionTexts.at(i));
    }
}

void MainWindow::fillDemoData() {
    // ===== 05_Single_HTML_Frontend_Reference.html 示例数据逐项同源 =====
    struct Demo { const char* name; int dur; };
    const std::initializer_list<Demo> playlist = {
        {"航拍中国 第三季 第01集.mp4", 2952}, {"典籍里的中国 第二季.mp4", 2706},
        {"河西走廊 第01集.mp4", 2841}, {"大国崛起 第01集.mp4", 2793},
        {"万里长城 第01集.mp4", 2897}};
    for (const Demo& d : playlist) {
        const QString name = QString::fromUtf8(d.name);
        m_mediaPaths.append(name);
        auto* item = new QListWidgetItem(m_mediaList);   // 文字留空：显示走行控件，
        item->setData(Qt::UserRole, name);               // 条目自带文字会被默认委托
        m_demoDurations.insert(name, qint64(d.dur) * 1000);   // 画在透明行控件底下（重影根因）
    }
    const std::initializer_list<Demo> history = {
        {"中国通史 第37集.mp4", 2860}, {"无字幕课程录播 04.mp4", 3925},
        {"采访素材_未剪辑.mov", 1634}};
    for (const Demo& d : history) {
        const QString name = QString::fromUtf8(d.name);
        auto* item = new QListWidgetItem(m_historyList);   // 同上：文字留空防重影
        item->setData(Qt::UserRole, name);
        m_demoDurations.insert(name, qint64(d.dur) * 1000);
    }
    refreshMediaRows();
    m_mediaList->setCurrentRow(0);   // 参考 state.activeIndex=0：首项紫色胶囊选中
    // 转写面板：transcriptSegments 7 段（时间 + 灰字预览 + 白字定稿）
    const struct { int start; const char* partial; const char* final; } segs[] = {
        {12, "在广袤的土地之上", "山川河流交错相连"},
        {18, "这里是中国", "美丽而辽阔的家园"},
        {24, "从高空俯瞰", "一幅壮丽的画卷徐徐展开"},
        {30, "每一寸土地都承载着", "自然与人文的故事"},
        {37, "晨光穿过云层", "照亮群山之间的河谷"},
        {45, "千百年来", "人们在这里繁衍生息"},
        {52, "今天的旅程", "将从这片云海开始"}};
    for (const auto& s : segs) {
        auto* item = new QListWidgetItem(m_transcript);
        auto* lab = new QLabel(QStringLiteral(
            "<div style='color:rgba(167,173,184,0.72);font-size:11px;'>%1</div>"
            "<div style='color:#757c88;'>%2</div><div>%3</div>")
                .arg(formatTime(s.start),
                     QString::fromUtf8(s.partial), QString::fromUtf8(s.final)));
        lab->setObjectName(QStringLiteral("transcriptItem"));
        lab->setTextFormat(Qt::RichText);
        lab->setWordWrap(true);
        item->setSizeHint(QSize(218, lab->heightForWidth(190) + 20));
        m_transcript->setItemWidget(item, lab);
    }
    m_statLines->setText(tr("字幕行数：128"));
    m_statLatency->setText(tr("实时延迟：1.2s"));
    const QString demoTitle = QStringLiteral("航拍中国 第三季 第01集.mp4");
    if (m_titleFile) m_titleFile->setText(demoTitle);
    // 参考稿 .video-file-title = stripExtension(fileName)，标题栏保留扩展名、画面内标题去除
    if (m_videoFileTitle) m_videoFileTitle->setText(QStringLiteral("航拍中国 第三季 第01集"));
    setWindowTitle(QStringLiteral("实时字幕播放器 — %1").arg(demoTitle));
    m_demoCurrent = 1458;   // state.current = 24:18
    m_seek->setRange(0, 2952);
    m_seek->setValue(m_demoCurrent);
    m_timeCur->setText(formatTime(1458));
    m_timeDur->setText(formatTime(2952));
    m_btnSpeed->setText(QStringLiteral("1.50x ⌄"));
    m_volume->setValue(72);   // state.volume 0.72
    m_demoTimer = new QTimer(this);
    connect(m_demoTimer, &QTimer::timeout, this, &MainWindow::demoTick);
    m_demoTimer->start(1000);
    demoTick();
}

void MainWindow::demoTick() {
    if (!m_demoMode) return;
    m_demoCurrent = (m_demoCurrent + 1) % 2952;
    m_seek->blockSignals(true);
    m_seek->setValue(m_demoCurrent);
    m_seek->blockSignals(false);
    m_timeCur->setText(formatTime(m_demoCurrent));
    // activeSegmentForTime 同源算法：秒数对 60 取模落段
    const int rel = ((m_demoCurrent % 60) + 60) % 60;
    const struct { int start; int end; const char* partial; const char* final; } segs[] = {
        {12, 17, "在广袤的土地之上", "山川河流交错相连"},
        {18, 23, "这里是中国", "美丽而辽阔的家园"},
        {24, 29, "从高空俯瞰", "一幅壮丽的画卷徐徐展开"},
        {30, 36, "每一寸土地都承载着", "自然与人文的故事"},
        {37, 44, "晨光穿过云层", "照亮群山之间的河谷"},
        {45, 51, "千百年来", "人们在这里繁衍生息"},
        {52, 59, "今天的旅程", "将从这片云海开始"}};
    int idx = -1;
    for (int i = 0; i < 7; ++i)
        if (rel >= segs[i].start && rel <= segs[i].end) { idx = i; break; }
    if (idx < 0) idx = qBound(0, rel * 7 / 60, 6);
    m_partialText = QString::fromUtf8(segs[idx].partial);
    m_overlayFinalText = QString::fromUtf8(segs[idx].final);
    if (!qEnvironmentVariableIsEmpty("RCP_NO_OVERLAY")) return;
    m_partialLabel->setText(m_partialText);
    m_partialLabel->setVisible(true);
    m_finalLabel->setText(m_overlayFinalText);
    if (m_video) m_video->update();
    // 转写面板当前句高亮（参考 .transcript-item.active 紫底）+ 滚动跟随
    if (idx != m_demoSegIdx) {
        m_demoSegIdx = idx;
        for (int r = 0; r < m_transcript->count(); ++r) {
            QWidget* wgt = m_transcript->itemWidget(m_transcript->item(r));
            if (wgt) wgt->setStyleSheet(r == idx
                ? QStringLiteral("background:#4a3f9c;border-radius:6px;")
                : QStringLiteral("background:transparent;"));
        }
        if (QListWidgetItem* cur = m_transcript->item(idx))
            m_transcript->scrollToItem(cur, QAbstractItemView::PositionAtCenter);
    }
}

void MainWindow::applyTheme() { qApp->setStyleSheet(QString::fromUtf8(kAppQss)); }

// ================= 标题栏 =================
void MainWindow::buildTitleBar(QWidget* parent) {
    m_titleBar = new QWidget(parent);
    m_titleBar->setObjectName(QStringLiteral("titleBar"));
    m_titleBar->setFixedHeight(48);

    auto* h = new QHBoxLayout(m_titleBar);
    h->setContentsMargins(18, 0, 0, 0);
    h->setSpacing(11);

    auto* logo = new QLabel(m_titleBar);
    logo->setFixedSize(25, 25);
    logo->setStyleSheet(QStringLiteral("background:transparent;"));
    QPixmap pm(QStringLiteral(":/logo.png"));
    logo->setPixmap(pm.scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto* brand = new QLabel(tr("实时字幕播放器"), m_titleBar);
    brand->setObjectName(QStringLiteral("brand"));

    m_titleFile = new QLabel(tr("未打开媒体"), m_titleBar);
    m_titleFile->setObjectName(QStringLiteral("titleFile"));
    m_titleFile->setMinimumWidth(220);

    auto* actions = new QWidget(m_titleBar);
    auto* ah = new QHBoxLayout(actions);
    ah->setContentsMargins(0, 0, 0, 0);
    ah->setSpacing(5);

    auto mkTitle = [actions](const QString& ico, const QString& text) {
        auto* b = new QPushButton(actions);
        b->setIcon(icon(ico, kMuted));
        b->setIconSize(QSize(17, 17));
        b->setText(text);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:transparent;color:#a7adb8;border:none;border-radius:6px;padding:0 10px;min-height:34px;font-size:12px;}"
            "QPushButton:hover{background:#222832;color:#f4f6fb;}"));
        return b;
    };
    auto* bOpenFile = mkTitle(QStringLiteral("folder-open"), tr("打开文件"));
    auto* bOpenFolder = mkTitle(QStringLiteral("folder"), tr("打开文件夹"));
    m_btnSettingsTitle = mkTitle(QStringLiteral("settings"), tr("设置"));
    m_titleActions = {bOpenFile, bOpenFolder, m_btnSettingsTitle};
    for (QPushButton* b : m_titleActions) m_titleActionTexts << b->text();
    connect(bOpenFile, &QPushButton::clicked, this, &MainWindow::onOpen);
    connect(bOpenFolder, &QPushButton::clicked, this, &MainWindow::onOpenFolder);
    connect(m_btnSettingsTitle, &QPushButton::clicked, this, &MainWindow::onToggleSettings);

    auto* sep = new QFrame(actions);
    sep->setFixedSize(1, 22);
    sep->setStyleSheet(QStringLiteral("background:rgba(255,255,255,0.105);margin:0 7px;"));

    auto mkWin = [actions](const QString& ico) {
        auto* b = new QPushButton(actions);
        b->setIcon(icon(ico, kMuted, 15));
        b->setIconSize(QSize(15, 15));
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(46, 47);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:transparent;border:none;border-radius:0;color:#a7adb8;}"
            "QPushButton:hover{background:#222832;color:#f4f6fb;}"));
        return b;
    };
    auto* bMin = mkWin(QStringLiteral("minus"));
    auto* bMax = mkWin(QStringLiteral("max"));
    auto* bClose = mkWin(QStringLiteral("close"));
    bClose->setStyleSheet(QStringLiteral(
        "QPushButton{background:transparent;border:none;color:#a7adb8;}"
        "QPushButton:hover{background:#c42b3a;color:#fff;}"));
    connect(bMin, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(bMax, &QPushButton::clicked, this, [this] { isMaximized() ? showNormal() : showMaximized(); });
    connect(bClose, &QPushButton::clicked, this, &QWidget::close);

    ah->addWidget(bOpenFile);
    ah->addWidget(bOpenFolder);
    ah->addWidget(m_btnSettingsTitle);
    ah->addWidget(sep);
    ah->addWidget(bMin);
    ah->addWidget(bMax);
    ah->addWidget(bClose);

    h->addWidget(logo);
    h->addWidget(brand);
    h->addSpacing(17);
    h->addWidget(m_titleFile, 1);
    h->addWidget(actions);
}

// ================= 左侧栏 =================
QWidget* MainWindow::buildLeftRail(QWidget* parent) {
    auto* rail = new QFrame(parent);
    rail->setObjectName(QStringLiteral("leftRail"));
    m_leftRail = rail;
    rail->setFixedWidth(238);

    auto* v = new QVBoxLayout(rail);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    auto* plSection = new QWidget(rail);
    plSection->setFixedHeight(276);
    auto* pv = new QVBoxLayout(plSection);
    pv->setContentsMargins(0, 0, 0, 0);
    pv->setSpacing(0);
    auto* head = new QWidget(plSection);
    head->setFixedHeight(42);
    auto* hh = new QHBoxLayout(head);
    hh->setContentsMargins(18, 0, 6, 0);
    m_libTitle = new QLabel(tr("播放列表"), head);
    m_libTitle->setProperty("class", "railTitle");
    auto* add = new QPushButton(head);
    add->setIcon(icon(QStringLiteral("plus"), kMuted));
    add->setIconSize(QSize(17, 17));
    add->setFixedSize(32, 32);
    add->setCursor(Qt::PointingHandCursor);
    add->setToolTip(tr("添加视频"));
    add->setStyleSheet(QStringLiteral(
        "QPushButton{background:transparent;border:none;border-radius:7px;}"
        "QPushButton:hover{background:#222832;}"));
    connect(add, &QPushButton::clicked, this, &MainWindow::onOpen);
    hh->addWidget(m_libTitle);
    hh->addStretch();
    hh->addWidget(add);
    pv->addWidget(head);

    // 顶部 276px 区：tabs 切换播放列表 / 历史记录（参考稿 library-view 结构）
    m_mediaList = new QListWidget(plSection);
    m_mediaList->setObjectName(QStringLiteral("playlist"));
    connect(m_mediaList, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistActivated);
    connect(m_mediaList, &QListWidget::currentRowChanged, this, [this] { refreshMediaRows(); });
    m_historyList = new QListWidget(plSection);
    m_historyList->setObjectName(QStringLiteral("history"));
    connect(m_historyList, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistActivated);
    m_mediaStack = new QStackedWidget(plSection);
    m_mediaStack->addWidget(m_mediaList);
    m_mediaStack->addWidget(m_historyList);
    pv->addWidget(m_mediaStack, 1);
    v->addWidget(plSection);

    auto* tabs = new QWidget(rail);
    tabs->setFixedHeight(42);
    auto* th = new QHBoxLayout(tabs);
    th->setContentsMargins(0, 0, 0, 0);
    th->setSpacing(0);
    m_tabPlaylist = new QPushButton(tr("播放列表"), tabs);
    m_tabHistory = new QPushButton(tr("历史记录"), tabs);
    const QString tabQss = QStringLiteral(
        "QPushButton{background:transparent;border:none;border-right:1px solid rgba(255,255,255,0.105);color:#a7adb8;font-size:12px;min-height:42px;border-radius:0;}"
        "QPushButton:hover{background:#222832;color:#f4f6fb;}"
        "QPushButton:checked{color:#7868ff;background:rgba(120,104,255,0.18);}");
    m_tabPlaylist->setStyleSheet(tabQss);
    m_tabHistory->setStyleSheet(tabQss + QStringLiteral("QPushButton{border-right:none;}"));
    m_tabPlaylist->setCheckable(true);
    m_tabHistory->setCheckable(true);
    m_tabPlaylist->setChecked(true);
    m_tabPlaylist->setCursor(Qt::PointingHandCursor);
    m_tabHistory->setCursor(Qt::PointingHandCursor);
    // 参考稿 rail-tab 带 15px list/history 图标，激活态随文字变 accent 色
    const auto syncTabIcons = [this] {
        const QColor accent(QStringLiteral("#7868ff"));
        m_tabPlaylist->setIcon(icon(QStringLiteral("list"),
                                    m_tabPlaylist->isChecked() ? accent : kMuted, 15));
        m_tabHistory->setIcon(icon(QStringLiteral("history"),
                                   m_tabHistory->isChecked() ? accent : kMuted, 15));
        m_tabPlaylist->setIconSize(QSize(15, 15));
        m_tabHistory->setIconSize(QSize(15, 15));
    };
    syncTabIcons();
    // 互斥组：checkable 按钮默认不互斥，点"历史记录"不会取消"播放列表"的选中，
    // toggled 不触发、栈不切换（用户报"点击历史记录没有反应"根因）。
    auto* tabGroup = new QButtonGroup(this);
    tabGroup->setExclusive(true);
    tabGroup->addButton(m_tabPlaylist);
    tabGroup->addButton(m_tabHistory);
    th->addWidget(m_tabPlaylist);
    th->addWidget(m_tabHistory);
    v->addWidget(tabs);

    connect(m_tabPlaylist, &QPushButton::toggled, this, [this, syncTabIcons](bool on) {
        m_mediaStack->setCurrentIndex(on ? 0 : 1);
        m_libTitle->setText(on ? tr("播放列表") : tr("历史记录"));
        syncTabIcons();
        if (on) refreshMediaRows();
    });
    connect(m_tabHistory, &QPushButton::toggled, this, [this, syncTabIcons](bool on) {
        m_mediaStack->setCurrentIndex(on ? 1 : 0);
        m_libTitle->setText(on ? tr("历史记录") : tr("播放列表"));
        syncTabIcons();
        if (on) refreshMediaRows();
    });

    auto* trHead = new QWidget(rail);
    trHead->setFixedHeight(42);
    auto* thh = new QHBoxLayout(trHead);
    thh->setContentsMargins(18, 0, 6, 0);
    auto* t2 = new QLabel(tr("实时字幕"), trHead);
    t2->setProperty("class", "railTitle");
    auto* clr = new QPushButton(trHead);
    clr->setIcon(icon(QStringLiteral("close"), kMuted, 15));
    clr->setIconSize(QSize(15, 15));
    clr->setFixedSize(32, 32);
    clr->setCursor(Qt::PointingHandCursor);
    clr->setToolTip(tr("清空字幕记录"));
    clr->setStyleSheet(add->styleSheet());
    connect(clr, &QPushButton::clicked, this, [this] {
        m_transcript->clear();
        m_lastPartialLabel = nullptr;
        m_finalCount = 0;
        m_statLines->setText(tr("字幕行数：0"));
    });
    thh->addWidget(t2);
    thh->addStretch();
    thh->addWidget(clr);
    v->addWidget(trHead);

    m_transcript = new QListWidget(rail);
    m_transcript->setObjectName(QStringLiteral("transcript"));
    m_transcript->setSelectionMode(QAbstractItemView::NoSelection);
    v->addWidget(m_transcript, 3);

    auto* footer = new QWidget(rail);
    footer->setFixedHeight(74);
    auto* fv = new QVBoxLayout(footer);
    fv->setContentsMargins(12, 10, 12, 10);
    fv->setSpacing(7);
    auto* search = new QLineEdit(footer);
    search->setObjectName(QStringLiteral("searchBox"));
    search->setPlaceholderText(tr("搜索字幕内容"));
    search->addAction(icon(QStringLiteral("search"), kDim, 15), QLineEdit::LeadingPosition);
    search->setClearButtonEnabled(true);
    connect(search, &QLineEdit::textChanged, this, &MainWindow::onTranscriptSearch);
    fv->addWidget(search);
    auto* stats = new QHBoxLayout();
    m_statLines = new QLabel(tr("字幕行数：0"), footer);
    m_statLatency = new QLabel(tr("实时延迟：—"), footer);
    for (auto* l : {m_statLines, m_statLatency}) l->setObjectName(QStringLiteral("railStats"));
    stats->addWidget(m_statLines);
    stats->addStretch();
    stats->addWidget(m_statLatency);
    fv->addLayout(stats);
    v->addWidget(footer);
    return rail;
}

// ================= 舞台 =================
QWidget* MainWindow::buildStage(QWidget* parent) {
    auto* stage = new QWidget(parent);
    stage->setStyleSheet(QStringLiteral("background:#0e1115;"));
    auto* v = new QVBoxLayout(stage);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    v->addWidget(buildVideoArea(stage), 1);
    v->addWidget(buildControlDeck(stage));
    return stage;
}

QWidget* MainWindow::buildVideoArea(QWidget* parent) {
    auto* surface = new QFrame(parent);
    surface->setObjectName(QStringLiteral("videoSurface"));
    auto* grid = new QGridLayout(surface);
    grid->setContentsMargins(0, 0, 0, 0);

    // 真实 GL 视频部件（RCP_SNAPSHOT 快照也走真实渲染，用于验证视频画面）。
    rcpMark("buildVideoArea:pre-m_video");
    m_video = new MpvRenderWidget(surface);
    rcpMark("buildVideoArea:m_video-created");
    m_video->setMinimumSize(320, 240);
    if (m_player && m_player->handle()) m_video->attachPlayer(m_player);
    rcpMark("buildVideoArea:attached");
    m_video->installEventFilter(this);
    grid->addWidget(m_video, 0, 0);

    m_videoHost = surface;
    surface->installEventFilter(this);

    QWidget* overlayParent = m_video ? qobject_cast<QWidget*>(m_video) : surface;

    m_captionOverlay = new QWidget(overlayParent);
    m_captionOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_captionOverlay->setStyleSheet(QStringLiteral("background:transparent;"));
    auto* cv = new QVBoxLayout(m_captionOverlay);
    cv->setContentsMargins(0, 0, 0, 0);
    cv->setSpacing(5);
    m_partialLabel = new CaptionLabel(m_captionOverlay);
    m_partialLabel->setCaptionStyle(CaptionLabel::Partial);
    m_finalLabel = new CaptionLabel(m_captionOverlay);
    m_finalLabel->setCaptionStyle(CaptionLabel::Final);
    cv->addStretch();
    cv->addWidget(m_partialLabel);
    cv->addWidget(m_finalLabel);

    m_waveform = new Waveform(overlayParent);

    // 渐晕层（.video-vignette）
    m_vignette = new QLabel(overlayParent);
    m_vignette->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_vignette->setStyleSheet(QStringLiteral(
        "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        " stop:0 rgba(5,7,10,0.20), stop:0.18 rgba(5,7,10,0),"
        " stop:0.65 rgba(5,7,10,0), stop:1 rgba(5,7,10,0.43));"));

    // 拖放提示（.drop-hint）
    m_dropHint = new QLabel(tr("松开即可打开视频"), overlayParent);
    m_dropHint->setAlignment(Qt::AlignCenter);
    m_dropHint->setStyleSheet(QStringLiteral(
        "border:2px dashed rgba(255,255,255,0.5); border-radius:12px;"
        "background:rgba(50,43,124,0.35); color:#fff; font-size:18px; font-weight:600;"));
    m_dropHint->hide();

    m_videoFileTitle = new QLabel(tr("未打开媒体"), overlayParent);
    m_videoFileTitle->setObjectName(QStringLiteral("videoFileTitle"));
    m_videoFileTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_videoFileTitle->move(18, 18);

    m_privacyBadge = new QWidget(overlayParent);
    m_privacyBadge->setObjectName(QStringLiteral("privacyBadge"));
    m_privacyBadge->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* ph = new QHBoxLayout(m_privacyBadge);
    ph->setContentsMargins(10, 5, 10, 5);
    ph->setSpacing(6);
    auto* dot = new QLabel(QStringLiteral("\u25CF"), m_privacyBadge);
    dot->setStyleSheet(QStringLiteral("color:%1;font-size:7px;background:transparent;").arg(kSuccess.name()));
    auto* ptxt = new QLabel(tr("本地离线识别"), m_privacyBadge);
    ph->addWidget(dot);
    ph->addWidget(ptxt);
    m_privacyBadge->adjustSize();

    return surface;
}


// ================= 控制台 =================
QWidget* MainWindow::buildControlDeck(QWidget* parent) {
    auto* deck = new QWidget(parent);
    deck->setObjectName(QStringLiteral("controlDeck"));
    deck->setFixedHeight(108);
    auto* v = new QVBoxLayout(deck);
    v->setContentsMargins(18, 8, 18, 10);
    v->setSpacing(4);

    auto* tl = new QHBoxLayout();
    tl->setSpacing(9);
    m_timeCur = new QLabel(tr("00:00"), deck);
    m_timeDur = new QLabel(tr("00:00"), deck);
    m_timeDur->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_seek = new QSlider(Qt::Horizontal, deck);
    m_seek->setRange(0, 0);
    m_seek->setCursor(Qt::PointingHandCursor);
    tl->addWidget(m_timeCur);
    tl->addWidget(m_seek, 1);
    tl->addWidget(m_timeDur);
    v->addLayout(tl);

    auto* row = new QHBoxLayout();
    row->setSpacing(10);
    QHBoxLayout *ll = nullptr, *cc = nullptr, *rr = nullptr;
    auto mkGroup = [deck](QHBoxLayout** out, int spacing) {
        auto* w = new QWidget(deck);
        w->setStyleSheet(QStringLiteral("background:transparent;"));
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(spacing);
        *out = h;
        return w;
    };
    auto* left = mkGroup(&ll, 8);
    auto* center = mkGroup(&cc, 12);
    auto* right = mkGroup(&rr, 8);

    auto cbtn = [deck](const QString& ico, const QString& tip, bool checkable = false, int px = 19) {
        auto* b = new QPushButton(deck);
        b->setIcon(icon(ico, QColor("#e6e8ee"), px));
        b->setIconSize(QSize(px, px));
        b->setFixedSize(34, 34);
        b->setToolTip(tip);
        b->setCursor(Qt::PointingHandCursor);
        b->setCheckable(checkable);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:transparent;border:none;border-radius:17px;}"
            "QPushButton:hover{background:rgba(255,255,255,0.08);}"
            "QPushButton:checked{background:rgba(121,105,255,0.22);}"));
        return b;
    };
    // 左组（参考稿：打开文件夹/截图/AB/字幕开关）
    auto* btnLib = cbtn(QStringLiteral("folder"), tr("打开文件夹"));
    m_btnCaption = cbtn(QStringLiteral("subtitle"), tr("开关实时字幕（R）"), true);
    m_btnCaption->setChecked(true);
    m_btnCam = cbtn(QStringLiteral("camera"), tr("截图"));
    m_btnAb = new QPushButton(QStringLiteral("AB"), deck);
    m_btnAb->setFixedSize(34, 34);
    m_btnAb->setToolTip(tr("设置 AB 循环"));
    m_btnAb->setCursor(Qt::PointingHandCursor);
    // 参考稿 AB 文本 12px/700
    m_btnAb->setStyleSheet(QStringLiteral(
        "QPushButton{background:transparent;border:none;border-radius:17px;font-size:12px;font-weight:700;}"
        "QPushButton:hover{background:rgba(255,255,255,0.08);}"
        "QPushButton:checked{background:rgba(121,105,255,0.22);}"));
    m_btnTrack = cbtn(QStringLiteral("subtitle"), tr("切换字幕轨"));

    m_btnB10 = cbtn(QStringLiteral("rewind"), tr("后退 10 秒"));
    m_btnPrev = cbtn(QStringLiteral("prev"), tr("上一个"));
    m_playBtn = new QPushButton(deck);
    m_playBtn->setObjectName(QStringLiteral("btnPlay"));
    m_playBtn->setIcon(icon(QStringLiteral("play"), QColor("#ffffff"), 22));
    m_playBtn->setIconSize(QSize(22, 22));
    m_playBtn->setFixedSize(45, 45);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #7c6aff,stop:1 #5848ce);border:none;border-radius:22px;}"
        "QPushButton:hover{background:#7c6aff;}"));
    m_btnNext = cbtn(QStringLiteral("next"), tr("下一个"));
    m_btnF10 = cbtn(QStringLiteral("forward"), tr("前进 10 秒"));

    m_btnSpeed = new QPushButton(deck);
    m_btnSpeed->setObjectName(QStringLiteral("btnSpeedText"));
    m_btnSpeed->setText(QStringLiteral("1.00x ⌄"));
    m_btnSpeed->setCursor(Qt::PointingHandCursor);
    m_btnSpeed->setStyleSheet(QStringLiteral(
        "QPushButton{background:transparent;border:none;border-radius:6px;color:#9c8fff;font-size:12px;min-height:34px;padding:0 5px;}"
        "QPushButton:hover{background:rgba(255,255,255,0.07);}"));

    m_btnSettings = cbtn(QStringLiteral("settings"), tr("设置（S）"));
    auto* volIcon = new QLabel(deck);
    volIcon->setStyleSheet(QStringLiteral("background:transparent;"));
    volIcon->setPixmap(icon(QStringLiteral("volume"), QColor("#e6e8ee"), 18).pixmap(18, 18));
    m_volume = new QSlider(Qt::Horizontal, deck);
    m_volume->setRange(0, 100);
    m_volume->setValue(100);
    m_volume->setFixedWidth(70);
    m_volume->setCursor(Qt::PointingHandCursor);
    m_btnFs = cbtn(QStringLiteral("fullscreen"), tr("全屏（F）"));

    ll->addWidget(btnLib);
    ll->addWidget(m_btnCam);
    ll->addWidget(m_btnAb);
    ll->addWidget(m_btnCaption);
    cc->addWidget(m_btnB10);
    cc->addWidget(m_btnPrev);
    cc->addWidget(m_playBtn);
    cc->addWidget(m_btnNext);
    cc->addWidget(m_btnF10);
    rr->addWidget(m_btnSpeed);
    rr->addWidget(volIcon);
    rr->addWidget(m_volume);
    rr->addWidget(m_btnTrack);   // 参考稿右组：倍速/音量/字幕轨(subtitle 图标)/设置/全屏
    rr->addWidget(m_btnSettings);
    rr->addWidget(m_btnFs);

    row->addWidget(left, 1);
    row->addWidget(center);
    row->addWidget(right, 1);
    v->addLayout(row);

    connect(m_btnCaption, &QPushButton::clicked, this, &MainWindow::onToggleCaption);
    connect(btnLib, &QPushButton::clicked, this, &MainWindow::onOpenFolder);
    connect(m_btnCam, &QPushButton::clicked, this, &MainWindow::onScreenshot);
    connect(m_btnAb, &QPushButton::clicked, this, &MainWindow::onAbLoop);
    connect(m_btnTrack, &QPushButton::clicked, this, &MainWindow::onCycleSub);
    connect(m_btnB10, &QPushButton::clicked, this, [this] { m_player->seek(qBound(0.0, m_player->timePosition() - 10.0, m_duration), false); });
    connect(m_btnF10, &QPushButton::clicked, this, [this] { m_player->seek(qBound(0.0, m_player->timePosition() + 10.0, m_duration), false); });
    connect(m_btnPrev, &QPushButton::clicked, this, [this] { onPrevNext(-1); });
    connect(m_btnNext, &QPushButton::clicked, this, [this] { onPrevNext(1); });
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_btnSettings, &QPushButton::clicked, this, &MainWindow::onToggleSettings);
    connect(m_btnFs, &QPushButton::clicked, this, &MainWindow::onToggleFullscreen);
    connect(m_volume, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(m_seek, &QSlider::sliderMoved, this, &MainWindow::onSeekSlider);
    connect(m_seek, &QSlider::sliderReleased, this, &MainWindow::onSeekReleased);
    connect(m_btnSpeed, &QPushButton::clicked, this, [this] {
        auto* pop = new QFrame(this, Qt::Popup);
        pop->setObjectName(QStringLiteral("speedPop"));
        pop->setStyleSheet(QStringLiteral(
            "QFrame{background:#191e25;border:1px solid rgba(255,255,255,0.16);border-radius:10px;}"
            "QLabel{background:transparent;color:#a7adb8;font-size:11px;}"));
        pop->setFixedWidth(250);
        auto* pv = new QVBoxLayout(pop);
        pv->setContentsMargins(12, 12, 12, 12);
        pv->setSpacing(8);
        pv->addWidget(new QLabel(tr("播放速度 · 保留音调"), pop));
        auto* grid = new QGridLayout();
        grid->setSpacing(6);
        const double speeds[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};
        for (int i = 0; i < 8; ++i) {
            auto* b = new QPushButton(QString::number(speeds[i], 'f', 2) + QStringLiteral("x"), pop);
            b->setStyleSheet(QStringLiteral(
                "QPushButton{background:#20252d;border:1px solid rgba(255,255,255,0.105);border-radius:6px;color:#a7adb8;font-size:12px;min-height:32px;}"
                "QPushButton:hover{border-color:rgba(121,105,255,0.55);color:#f4f6fb;}"));
            connect(b, &QPushButton::clicked, pop, [this, pop, v = speeds[i]] {
                onSpeed(v);
                pop->close();
            });
            grid->addWidget(b, i / 4, i % 4);
        }
        pv->addLayout(grid);
        pop->move(m_btnSpeed->mapToGlobal(QPoint(0, -150)));
        pop->show();
    });
    return deck;
}

// ================= 设置面板 =================
QWidget* MainWindow::buildSettingsPanel(QWidget* parent) {
    auto* panel = new QFrame(parent);
    panel->setObjectName(QStringLiteral("settingsPanel"));
    panel->setFixedWidth(344);
    auto* v = new QVBoxLayout(panel);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    auto* head = new QWidget(panel);
    head->setFixedHeight(48);
    head->setStyleSheet(QStringLiteral("background:transparent;border-bottom:1px solid rgba(255,255,255,0.105);"));
    auto* hh = new QHBoxLayout(head);
    hh->setContentsMargins(18, 0, 10, 0);
    auto* t = new QLabel(tr("设置"), head);
    t->setObjectName(QStringLiteral("settingsTitle"));
    auto* close = new QPushButton(head);
    close->setIcon(icon(QStringLiteral("close"), kMuted, 15));
    close->setIconSize(QSize(15, 15));
    close->setFixedSize(32, 32);
    close->setCursor(Qt::PointingHandCursor);
    close->setStyleSheet(QStringLiteral(
        "QPushButton{background:transparent;border:none;border-radius:7px;}"
        "QPushButton:hover{background:#222832;}"));
    connect(close, &QPushButton::clicked, this, &MainWindow::onToggleSettings);
    hh->addWidget(t);
    hh->addStretch();
    hh->addWidget(close);
    v->addWidget(head);

    auto* body = new QWidget(panel);
    auto* bh = new QHBoxLayout(body);
    bh->setContentsMargins(0, 0, 0, 0);
    bh->setSpacing(0);

    auto* nav = new QWidget(body);
    nav->setFixedWidth(98);
    nav->setStyleSheet(QStringLiteral("background:transparent;border-right:1px solid rgba(255,255,255,0.105);"));
    auto* nv = new QVBoxLayout(nav);
    nv->setContentsMargins(6, 9, 6, 9);
    nv->setSpacing(2);
    const QList<QPair<const char*, QString>> pages = {
        {"settings", tr("常规")}, {"monitor", tr("播放")}, {"subtitle", tr("字幕")},
        {"sliders", tr("实时字幕")}, {"keyboard", tr("快捷键")}, {"palette", tr("外观")},
        {"info", tr("高级")}};
    int idx = 0;
    for (const auto& [ico, text] : pages) {
        auto* b = new QPushButton(icon(ico, kMuted, 17), text, nav);
        b->setCheckable(true);
        b->setIconSize(QSize(17, 17));
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:transparent;border:none;border-radius:6px;color:#a7adb8;font-size:11px;min-height:38px;text-align:left;padding:0 10px;}"
            "QPushButton:hover{background:#222832;color:#f4f6fb;}"
            "QPushButton:checked{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 rgba(82,72,163,0.78),stop:1 rgba(63,56,133,0.72));color:#fff;}"));
        const int my = idx;
        connect(b, &QPushButton::clicked, this, [this, my] {
            m_panes->setCurrentIndex(my);
            for (auto& [btn, i] : m_navBtns) btn->setChecked(i == my);
        });
        nv->addWidget(b);
        m_navBtns.append({b, idx});
        ++idx;
    }
    nv->addStretch();
    bh->addWidget(nav);

    m_panes = new QStackedWidget(body);
    m_panes->addWidget(wrapPaneScroll(paneGeneral()));
    m_panes->addWidget(wrapPaneScroll(panePlayback()));
    m_panes->addWidget(wrapPaneScroll(paneSubtitle()));
    m_panes->addWidget(wrapPaneScroll(paneRealtime()));
    m_panes->addWidget(wrapPaneScroll(paneShortcuts()));
    m_panes->addWidget(wrapPaneScroll(paneAppearance()));
    m_panes->addWidget(wrapPaneScroll(paneAdvanced()));
    bh->addWidget(m_panes, 1);
    v->addWidget(body, 1);

    if (!m_navBtns.isEmpty()) m_navBtns.first().first->setChecked(true);
    return panel;
}

QWidget* MainWindow::wrapPaneScroll(QWidget* inner) {
    inner->setObjectName(QStringLiteral("paneBody"));
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(inner);
    scroll->setStyleSheet(QStringLiteral(
        "QScrollArea{background:transparent;border:none;}"
        "QScrollBar:vertical{background:transparent;width:6px;margin:0;}"
        "QScrollBar::handle:vertical{background:rgba(255,255,255,0.18);border-radius:3px;min-height:30px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"));
    return scroll;
}

QWidget* MainWindow::paneGeneral() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(17, 17, 17, 28);
    v->setSpacing(13);
    auto* title = new QLabel(tr("常规"), w);
    title->setProperty("class", "paneTitle");
    v->addWidget(title);
    v->addWidget(sectionTitle(tr("启动与文件"), w));

    auto mkSwitch = [this, w](const QString& key, bool def) {
        auto* s = new Switch(w);
        s->setChecked(m_settings.value(key, def).toBool());
        connect(s, &Switch::toggled, this, [this, key](bool on) { m_settings.setValue(key, on); });
        return s;
    };
    v->addWidget(settingRow(tr("启动时恢复上次会话"),
                            mkSwitch(QStringLiteral("general/resumeSession"), true), w));
    v->addWidget(settingRow(tr("记住播放位置"),
                            mkSwitch(QStringLiteral("general/rememberPosition"), true), w));
    v->addWidget(settingRow(tr("打开文件后自动播放"),
                            mkSwitch(QStringLiteral("general/autoPlay"), true), w));
    v->addWidget(sectionTitle(tr("默认播放器"), w));
    auto* openDefaults = new QPushButton(tr("打开默认应用设置"), w);
    openDefaults->setCursor(Qt::PointingHandCursor);
    openDefaults->setStyleSheet(QStringLiteral(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #5147aa,stop:1 #6254cb);border:none;border-radius:6px;color:#fff;font-size:10px;min-height:29px;padding:0 10px;}"
        "QPushButton:hover{background:#6254cb;}"));
    connect(openDefaults, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
    });
    v->addWidget(settingRow(tr("在 Windows 默认应用中选择本播放器"), openDefaults, w,
                            tr("系统会要求用户确认，不修改受保护的 UserChoice。")));
    v->addWidget(sectionTitle(tr("隐私"), w));
    auto* offline = new Switch(w);
    offline->setChecked(true);
    offline->setEnabled(false);
    v->addWidget(settingRow(tr("离线模式（默认开启，无任何网络通信）"), offline, w));
    auto* crash = new Switch(w);
    crash->setChecked(false);
    crash->setEnabled(false);
    v->addWidget(settingRow(tr("发送匿名崩溃报告"), crash, w, tr("本应用无遥测。")));
    v->addStretch();
    return w;
}

QWidget* MainWindow::panePlayback() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(17, 17, 17, 28);
    v->setSpacing(13);
    auto* title = new QLabel(tr("播放"), w);
    title->setProperty("class", "paneTitle");
    v->addWidget(title);
    v->addWidget(sectionTitle(tr("播放行为"), w));
    auto* pitch = new Switch(w);
    pitch->setChecked(true);
    v->addWidget(settingRow(tr("保留音调"), pitch, w));
    auto* folderCont = new Switch(w);
    folderCont->setChecked(m_settings.value(QStringLiteral("playback/folderContinue"), true).toBool());
    connect(folderCont, &Switch::toggled, this, [this](bool on) {
        m_settings.setValue(QStringLiteral("playback/folderContinue"), on);
    });
    v->addWidget(settingRow(tr("文件夹连续播放"), folderCont, w));
    auto* speedSel = new QComboBox(w);
    speedSel->setProperty("class", "fieldSelect");
    speedSel->setFixedWidth(135);
    speedSel->addItems({tr("1.00x"), tr("1.25x"), tr("1.50x"), tr("2.00x")});
    const double def = m_settings.value(QStringLiteral("playback/speed"), 1.0).toDouble();
    speedSel->setCurrentIndex(def > 1.9 ? 3 : def > 1.4 ? 2 : def > 1.1 ? 1 : 0);
    connect(speedSel, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("playback/speed"),
                            i == 3 ? 2.0 : i == 2 ? 1.5 : i == 1 ? 1.25 : 1.0);
    });
    v->addWidget(settingRow(tr("默认倍速"), speedSel, w));
    v->addWidget(sectionTitle(tr("解码与画面"), w));
    auto* hw = new QComboBox(w);
    hw->setProperty("class", "fieldSelect");
    hw->setFixedWidth(135);
    hw->addItems({tr("关闭"), tr("自动"), tr("DXVA2"), tr("D3D11VA")});
    const QString cur = m_settings.value(QStringLiteral("playback/hwdec"), QStringLiteral("no")).toString();
    hw->setCurrentIndex(cur == QStringLiteral("auto") ? 1
                        : cur == QStringLiteral("dxva2") ? 2
                        : cur == QStringLiteral("d3d11va") ? 3 : 0);
    connect(hw, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("playback/hwdec"),
                            i == 1 ? QStringLiteral("auto")
                                   : i == 2 ? QStringLiteral("dxva2")
                                            : i == 3 ? QStringLiteral("d3d11va") : QStringLiteral("no"));
    });
    v->addWidget(settingRow(tr("硬件解码（下次启动生效）"), hw, w));
    v->addStretch();
    return w;
}

QWidget* MainWindow::paneSubtitle() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(17, 17, 17, 28);
    v->setSpacing(13);
    auto* title = new QLabel(tr("字幕"), w);
    title->setProperty("class", "paneTitle");
    v->addWidget(title);
    v->addWidget(sectionTitle(tr("同步"), w));
    auto* row = new QWidget(w);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    const QString smallBtnQss = QStringLiteral(
        "QPushButton{background:#20252d;border:1px solid rgba(255,255,255,0.16);border-radius:6px;color:#a7adb8;font-size:10px;min-height:29px;padding:0 10px;}"
        "QPushButton:hover{background:#222832;color:#f4f6fb;}");
    auto mkBtn = [w, smallBtnQss](const QString& t) {
        auto* b = new QPushButton(t, w);
        b->setStyleSheet(smallBtnQss);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };
    auto* minus = mkBtn(QStringLiteral("−"));
    minus->setFixedWidth(28);
    auto* plus = mkBtn(QStringLiteral("+"));
    plus->setFixedWidth(28);
    auto* val = new QLabel(tr("0.0 秒"), w);
    val->setObjectName(QStringLiteral("railStats"));
    auto apply = [this, val](double d) {
        m_delayMs = qBound(-5000LL, m_delayMs + qRound64(d * 1000), 5000LL);
        val->setText(tr("%1 秒").arg(m_delayMs / 1000.0, 0, 'f', 1));
    };
    connect(minus, &QPushButton::clicked, this, [apply] { apply(-0.1); });
    connect(plus, &QPushButton::clicked, this, [apply] { apply(0.1); });
    auto* reset = mkBtn(tr("重置"));
    auto* loadSub = mkBtn(tr("加载外挂字幕文件"));
    connect(reset, &QPushButton::clicked, this, [this, val] {
        m_delayMs = 0;
        val->setText(tr("0.0 秒"));
    });
    connect(loadSub, &QPushButton::clicked, this, [this] {
        const QString p = QFileDialog::getOpenFileName(this, tr("加载字幕"), QString(),
                                                       tr("字幕 (*.srt *.ass *.ssa *.vtt)"));
        if (!p.isEmpty()) m_player->loadSubtitle(p);
    });
    h->addWidget(minus);
    h->addWidget(plus);
    h->addWidget(val, 1);
    h->addWidget(reset);
    v->addWidget(settingRow(tr("字幕延迟"), row, w));
    v->addWidget(loadSub);
    v->addStretch();
    return w;
}

QWidget* MainWindow::paneRealtime() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(17, 17, 17, 28);
    v->setSpacing(13);
    auto* title = new QLabel(tr("实时字幕"), w);
    title->setProperty("class", "paneTitle");
    v->addWidget(title);
    // 运行状态（原视频区浮动卡合并至此，消除重复；数据全部真实）
    v->addWidget(sectionTitle(tr("运行状态"), w));
    auto* stRow = new QWidget(w);
    auto* sth = new QHBoxLayout(stRow);
    sth->setContentsMargins(0, 0, 0, 0);
    sth->setSpacing(6);
    m_asrDot = new QLabel(QStringLiteral("\u25CF"), stRow);
    m_asrDot->setStyleSheet(QStringLiteral("color:#69717d;font-size:7px;background:transparent;"));
    m_asrStatus = new QLabel(tr("未运行"), stRow);
    m_asrStatus->setStyleSheet(QStringLiteral("color:#69717d;font-size:11px;background:transparent;"));
    sth->addWidget(m_asrDot);
    sth->addWidget(m_asrStatus);
    sth->addStretch();
    v->addWidget(stRow);
    auto metricRow = [w](const QString& k, QLabel** val) {
        auto* row = new QWidget(w);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(6);
        auto* kl = new QLabel(k, row);
        kl->setProperty("class", "rowLabel");
        *val = new QLabel(QStringLiteral("—"), row);
        (*val)->setProperty("class", "rowLabel");
        h->addWidget(kl);
        h->addStretch();
        h->addWidget(*val);
        return row;
    };
    v->addWidget(metricRow(tr("延迟"), &m_asrLatency));
    v->addWidget(metricRow(tr("已识别时长"), &m_asrRecognized));
    v->addWidget(metricRow(tr("置信度"), &m_asrConf));
    auto* ex = new QPushButton(tr("导出字幕（SRT）"), w);
    ex->setCursor(Qt::PointingHandCursor);
    ex->setStyleSheet(QStringLiteral(
        "QPushButton{background:transparent;border:1px solid rgba(255,255,255,0.14);"
        "border-radius:6px;color:#9e91ff;min-height:30px;font-size:11px;}"
        "QPushButton:hover{background:rgba(121,105,255,0.13);}"));
    connect(ex, &QPushButton::clicked, this, &MainWindow::onExportSrt);
    v->addWidget(ex);

    v->addWidget(sectionTitle(tr("总开关"), w));
    auto* sw = new Switch(w);
    sw->setChecked(m_captionOn);
    connect(sw, &Switch::toggled, this, [this](bool on) {
        if (m_captionOn != on) m_btnCaption->setChecked(on);   // 触发 onToggleCaption
    });
    connect(m_btnCaption, &QPushButton::toggled, sw, &Switch::setChecked);
    v->addWidget(settingRow(tr("启用实时字幕"), sw, w));
    v->addWidget(sectionTitle(tr("引擎设置"), w));
    auto mkSel = [w](const QStringList& items, int disabled = -1) {
        auto* c = new QComboBox(w);
        c->setProperty("class", "fieldSelect");
        c->setFixedWidth(135);
        c->addItems(items);
        if (disabled >= 0) {
            auto* model = qobject_cast<QStandardItemModel*>(c->model());
            if (model) {
                QStandardItem* it = model->item(disabled);
                it->setEnabled(false);
            }
        }
        return c;
    };
    auto* engineSel = mkSel({tr("本地（SenseVoice）"), tr("本地（Paraformer）"), tr("云端高精度")});
    connect(engineSel, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("caption/engine"), i);
    });
    engineSel->setCurrentIndex(m_settings.value(QStringLiteral("caption/engine"), 0).toInt());
    v->addWidget(settingRow(tr("识别引擎"), engineSel, w));
    auto* langSel = mkSel({tr("中文（auto）"), tr("English"), tr("日本語（实验性）"),
                           tr("한국어（实验性）")});
    langSel->setCurrentIndex(m_settings.value(QStringLiteral("caption/lang"), 0).toInt());
    connect(langSel, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("caption/lang"), i);
    });
    v->addWidget(settingRow(tr("语言"), langSel, w));
    // 参考：Lite（低配置）/ Balanced（推荐，默认选中）
    auto* modelSel = mkSel({tr("Lite（低配置）"), tr("Balanced（推荐）")});
    modelSel->setCurrentIndex(m_settings.value(QStringLiteral("caption/model"), 1).toInt());
    connect(modelSel, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("caption/model"), i);
    });
    v->addWidget(settingRow(tr("模型大小"), modelSel, w));
    auto* devSel = mkSel({tr("自动（CPU / GPU）"), tr("CPU"), tr("GPU")});
    devSel->setCurrentIndex(m_settings.value(QStringLiteral("caption/device"), 0).toInt());
    connect(devSel, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("caption/device"), i);
    });
    v->addWidget(settingRow(tr("计算设备"), devSel, w));
    auto* spk = new Switch(w);
    spk->setEnabled(false);
    v->addWidget(settingRow(tr("使用说话人分离"), spk, w,
                            tr("说话人分离属于后续版本，保留禁用状态，避免展示无实现的假功能。")));
    v->addWidget(sectionTitle(tr("显示设置"), w));
    auto* capFontSel = mkSel({tr("思源黑体"), tr("微软雅黑"), tr("系统默认")});
    capFontSel->setCurrentIndex(m_settings.value(QStringLiteral("caption/font"), 0).toInt());
    connect(capFontSel, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("caption/font"), i);
        applyCaptionStyle();
    });
    v->addWidget(settingRow(tr("字幕字体"), capFontSel, w));
    auto* fontRow = new QWidget(w);
    auto* fh = new QHBoxLayout(fontRow);
    fh->setContentsMargins(0, 0, 0, 0);
    fh->setSpacing(6);
    const QString smallBtnQss2 = QStringLiteral(
        "QPushButton{background:#20252d;border:1px solid rgba(255,255,255,0.16);border-radius:6px;color:#a7adb8;font-size:10px;min-height:29px;}"
        "QPushButton:hover{background:#222832;color:#f4f6fb;}");
    auto mkSmall = [w, smallBtnQss2](const QString& t) {
        auto* b = new QPushButton(t, w);
        b->setStyleSheet(smallBtnQss2);
        b->setFixedWidth(28);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };
    auto* fminus = mkSmall(QStringLiteral("−"));
    auto* fplus = mkSmall(QStringLiteral("+"));
    auto applySize = [this](int d) {
        const int s = qBound(22, m_settings.value(QStringLiteral("caption/size"), 36).toInt() + d, 54);
        m_settings.setValue(QStringLiteral("caption/size"), s);
        applyCaptionStyle();
    };
    connect(fminus, &QPushButton::clicked, this, [applySize] { applySize(-1); });
    connect(fplus, &QPushButton::clicked, this, [applySize] { applySize(1); });
    fh->addWidget(fminus);
    fh->addWidget(fplus);
    v->addWidget(settingRow(tr("字号"), fontRow, w));
    // 字幕颜色：白/黄 segmented（参考 .segmented，黄=#ffe66d）
    auto* seg = new QWidget(w);
    auto* segh = new QHBoxLayout(seg);
    segh->setContentsMargins(0, 0, 0, 0);
    segh->setSpacing(0);
    auto mkSeg = [seg](const QString& t, bool active) {
        auto* b = new QPushButton(t, seg);
        b->setCheckable(true);
        b->setChecked(active);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton{background:#20252d;border:1px solid rgba(255,255,255,0.16);"
            "color:#a7adb8;font-size:11px;padding:4px 16px;}"
            "QPushButton:hover{background:#222832;}"
            "QPushButton:checked{background:#6756e6;border-color:#6756e6;color:#ffffff;}"));
        return b;
    };
    auto* segWhite = mkSeg(tr("白色"), true);
    auto* segYellow = mkSeg(tr("黄色"), false);
    auto* segGroup = new QButtonGroup(seg);
    segGroup->setExclusive(true);
    segGroup->addButton(segWhite);
    segGroup->addButton(segYellow);
    segh->addWidget(segWhite);
    segh->addWidget(segYellow);
    segh->addStretch();
    const bool yellow = m_settings.value(QStringLiteral("caption/color"), 0).toInt() == 1;
    segYellow->setChecked(yellow);
    segWhite->setChecked(!yellow);
    connect(segWhite, &QPushButton::toggled, this, [this](bool on) {
        if (on) { m_settings.setValue(QStringLiteral("caption/color"), 0); applyCaptionStyle(); }
    });
    connect(segYellow, &QPushButton::toggled, this, [this](bool on) {
        if (on) { m_settings.setValue(QStringLiteral("caption/color"), 1); applyCaptionStyle(); }
    });
    v->addWidget(settingRow(tr("字幕颜色"), seg, w));
    // 描边 / 阴影 select（0描边 1阴影 2关闭）
    auto* outlineSel = mkSel({tr("描边"), tr("阴影"), tr("关闭")});
    outlineSel->setCurrentIndex(m_settings.value(QStringLiteral("caption/outline"), 0).toInt());
    connect(outlineSel, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("caption/outline"), i);
        applyCaptionStyle();
    });
    v->addWidget(settingRow(tr("描边 / 阴影"), outlineSel, w));
    auto* pos = new QComboBox(w);
    pos->setProperty("class", "fieldSelect");
    pos->setFixedWidth(135);
    pos->addItems({tr("底部居中"), tr("画面中部"), tr("顶部居中")});
    pos->setCurrentIndex(m_settings.value(QStringLiteral("caption/position"), 0).toInt());
    connect(pos, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("caption/position"), i);
        repositionOverlays();
    });
    v->addWidget(settingRow(tr("字幕位置"), pos, w));
    auto* partialSw = new Switch(w);
    partialSw->setChecked(m_settings.value(QStringLiteral("caption/showPartial"), true).toBool());
    connect(partialSw, &Switch::toggled, this, [this](bool on) {
        m_settings.setValue(QStringLiteral("caption/showPartial"), on);
        updateOverlay();
    });
    v->addWidget(settingRow(tr("显示预览结果（灰字）"), partialSw, w));
    // 预览结果颜色 chip（参考 .color-chip #b8bac0）
    auto* chip = new QLabel(w);
    chip->setFixedSize(26, 15);
    chip->setStyleSheet(QStringLiteral("background:#b8bac0;border-radius:3px;"));
    v->addWidget(settingRow(tr("预览结果颜色"), chip, w));

    v->addWidget(sectionTitle(tr("导出设置"), w));
    auto* autoSw = new Switch(w);
    autoSw->setChecked(m_settings.value(QStringLiteral("caption/autoExport"), false).toBool());
    connect(autoSw, &Switch::toggled, this, [this](bool on) {
        m_settings.setValue(QStringLiteral("caption/autoExport"), on);
    });
    v->addWidget(settingRow(tr("自动导出"), autoSw, w));
    auto* fmtSel = mkSel({tr("SRT"), tr("VTT"), tr("纯文本")});
    fmtSel->setCurrentIndex(m_settings.value(QStringLiteral("caption/format"), 0).toInt());
    connect(fmtSel, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_settings.setValue(QStringLiteral("caption/format"), i);
    });
    v->addWidget(settingRow(tr("导出格式"), fmtSel, w));
    auto* dirRow = new QWidget(w);
    auto* dh = new QVBoxLayout(dirRow);
    dh->setContentsMargins(0, 0, 0, 0);
    dh->setSpacing(6);
    auto* dirLab = new QLabel(tr("保存目录"), dirRow);
    dirLab->setProperty("class", "rowLabel");
    dh->addWidget(dirLab);
    auto* dirInner = new QWidget(dirRow);
    auto* dih = new QHBoxLayout(dirInner);
    dih->setContentsMargins(0, 0, 0, 0);
    dih->setSpacing(6);
    auto* dirInput = new QLineEdit(dirInner);
    dirInput->setObjectName(QStringLiteral("fieldInput"));
    dirInput->setText(m_settings.value(QStringLiteral("caption/exportDir"),
                                       QStringLiteral("D:\\字幕导出")).toString());
    connect(dirInput, &QLineEdit::textChanged, this, [this](const QString& t) {
        m_settings.setValue(QStringLiteral("caption/exportDir"), t);
    });
    auto* dirBtn = new QPushButton(dirInner);
    dirBtn->setIcon(icon(QStringLiteral("folder"), QColor("#a7adb8"), 14));
    dirBtn->setFixedSize(29, 29);
    dirBtn->setCursor(Qt::PointingHandCursor);
    dirBtn->setStyleSheet(smallBtnQss2);
    connect(dirBtn, &QPushButton::clicked, this, [this, dirInput] {
        const QString d = QFileDialog::getExistingDirectory(this, tr("选择目录"), dirInput->text());
        if (!d.isEmpty()) dirInput->setText(d);
    });
    dih->addWidget(dirInput, 1);
    dih->addWidget(dirBtn);
    dh->addWidget(dirInner);
    v->addWidget(dirRow);
    v->addStretch();
    return w;
}

QWidget* MainWindow::paneShortcuts() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(17, 17, 17, 28);
    v->setSpacing(6);
    auto* title = new QLabel(tr("快捷键"), w);
    title->setProperty("class", "paneTitle");
    v->addWidget(title);
    const QList<QPair<QString, QString>> rows = {
        {tr("播放 / 暂停"), QStringLiteral("Space")},
        {tr("前进 5 秒"), QStringLiteral("→")},
        {tr("后退 5 秒"), QStringLiteral("←")},
        {tr("切换实时字幕"), QStringLiteral("R")},
        {tr("打开设置"), QStringLiteral("S")},
        {tr("全屏"), QStringLiteral("F")},
        {tr("打开文件"), QStringLiteral("Ctrl+O")}};
    for (const auto& [act, key] : rows) {
        auto* r = new QWidget(w);
        auto* h = new QHBoxLayout(r);
        h->setContentsMargins(0, 0, 0, 0);
        auto* a = new QLabel(act, r);
        a->setProperty("class", "rowLabel");
        auto* k = new QLabel(key, r);
        k->setStyleSheet(QStringLiteral(
            "background:#20252d;border:1px solid rgba(255,255,255,0.16);border-radius:4px;color:#e6e8ee;font-size:10px;padding:2px 8px;"));
        h->addWidget(a);
        h->addStretch();
        h->addWidget(k);
        v->addWidget(r);
    }
    v->addStretch();
    return w;
}

QWidget* MainWindow::paneAppearance() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(17, 17, 17, 28);
    v->setSpacing(13);
    auto* title = new QLabel(tr("外观"), w);
    title->setProperty("class", "paneTitle");
    v->addWidget(title);
    v->addWidget(sectionTitle(tr("界面"), w));
    auto* note = new QLabel(tr("深色主题为当前版本唯一主题；浅色主题属后续版本。"), w);
    note->setProperty("class", "rowHelp");
    note->setWordWrap(true);
    v->addWidget(note);
    v->addStretch();
    return w;
}

QWidget* MainWindow::paneAdvanced() {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(17, 17, 17, 28);
    v->setSpacing(13);
    auto* title = new QLabel(tr("高级"), w);
    title->setProperty("class", "paneTitle");
    v->addWidget(title);
    v->addWidget(sectionTitle(tr("模型与缓存"), w));
    auto* models = new QLabel(tr("模型随安装包内置（安装目录 models\\），离线可用，无需下载。"), w);
    models->setProperty("class", "rowHelp");
    models->setWordWrap(true);
    v->addWidget(settingRow(tr("模型目录"), models, w));
    v->addWidget(sectionTitle(tr("诊断"), w));
    auto* logs = new QPushButton(tr("打开应用数据目录"), w);
    logs->setStyleSheet(QStringLiteral(
        "QPushButton{background:#20252d;border:1px solid rgba(255,255,255,0.16);border-radius:6px;color:#a7adb8;font-size:10px;min-height:29px;padding:0 10px;}"
        "QPushButton:hover{background:#222832;color:#f4f6fb;}"));
    logs->setCursor(Qt::PointingHandCursor);
    connect(logs, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)));
    });
    v->addWidget(settingRow(tr("诊断"), logs, w));
    v->addStretch();
    return w;
}

QWidget* MainWindow::sectionTitle(const QString& text, QWidget* parent) {
    auto* box = new QWidget(parent);
    auto* vv = new QVBoxLayout(box);
    vv->setContentsMargins(0, 0, 0, 9);
    vv->setSpacing(9);
    auto* t = new QLabel(text, box);
    t->setProperty("class", "secTitle");
    vv->addWidget(t);
    auto* hr = new QFrame(box);
    hr->setFixedHeight(1);
    hr->setStyleSheet(QStringLiteral("background:rgba(255,255,255,0.105);border:none;"));
    vv->addWidget(hr);
    return box;
}

QWidget* MainWindow::settingRow(const QString& text, QWidget* editor, QWidget* parent,
                                const QString& help) {
    auto* row = new QWidget(parent);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(10);
    auto* col = new QWidget(row);
    auto* cv = new QVBoxLayout(col);
    cv->setContentsMargins(0, 0, 0, 0);
    cv->setSpacing(2);
    auto* l = new QLabel(text, col);
    l->setProperty("class", "rowLabel");
    l->setWordWrap(true);
    cv->addWidget(l);
    if (!help.isEmpty()) {
        auto* hl = new QLabel(help, col);
        hl->setProperty("class", "rowHelp");
        hl->setWordWrap(true);
        cv->addWidget(hl);
    }
    h->addWidget(col, 1);
    h->addWidget(editor);
    return row;
}

// ================= 转写/统计 =================
void MainWindow::appendTranscriptPartial(const QString& text) {
    rcpTrace(QStringLiteral("caption-partial %1").arg(text.left(30)));
    if (text.isEmpty()) return;
    const QString html = QStringLiteral(
        "<span style='color:#757c88;'>… </span><span>%1</span>").arg(text.toHtmlEscaped());
    if (m_lastPartialLabel) {
        m_lastPartialLabel->setText(html);
        m_transcript->scrollToBottom();
        return;
    }
    auto* item = new QListWidgetItem(m_transcript);
    auto* lab = new QLabel(html);
    lab->setObjectName(QStringLiteral("transcriptItem"));
    lab->setTextFormat(Qt::RichText);
    lab->setWordWrap(true);
    item->setSizeHint(QSize(218, lab->heightForWidth(190) + 20));
    m_transcript->setItemWidget(item, lab);
    m_lastPartialLabel = lab;
    m_transcript->scrollToBottom();
}

void MainWindow::appendTranscriptFinal(long long startMs, const QString& text) {
    // 字幕证据输出到 trace（RCP_TRACE 启用时）——验证实时字幕链路。
    rcpTrace(QStringLiteral("caption-final #%1 [%2ms] %3").arg(m_finalCount + 1).arg(startMs).arg(text));
    const int last = m_transcript->count() - 1;
    if (last >= 0 && m_lastPartialLabel &&
        m_transcript->itemWidget(m_transcript->item(last)) == m_lastPartialLabel.data()) {
        delete m_transcript->item(last);
        m_lastPartialLabel = nullptr;
    }
    auto* item = new QListWidgetItem(m_transcript);
    auto* lab = new QLabel(QStringLiteral(
        "<div style='color:rgba(167,173,184,0.72);font-size:11px;'>%1</div>"
        "<div>%2</div>").arg(startMs >= 0 ? formatTime(startMs / 1000.0) : QString(),
                             text.toHtmlEscaped()));
    lab->setObjectName(QStringLiteral("transcriptItem"));
    lab->setTextFormat(Qt::RichText);
    lab->setWordWrap(true);
    item->setSizeHint(QSize(218, lab->heightForWidth(190) + 22));
    m_transcript->setItemWidget(item, lab);
    ++m_finalCount;
    m_statLines->setText(tr("字幕行数：%1").arg(m_finalCount));
    m_transcript->scrollToBottom();
}

void MainWindow::setAsrStatus(const QString& text, const QString& color) {
    m_asrDot->setStyleSheet(QStringLiteral("color:%1;font-size:7px;background:transparent;").arg(color));
    m_asrStatus->setText(text);
    m_asrStatus->setStyleSheet(QStringLiteral("color:%1;font-size:11px;background:transparent;").arg(color));
}

void MainWindow::onTranscriptSearch(const QString& text) {
    for (int i = 0; i < m_transcript->count(); ++i) {
        auto* it = m_transcript->item(i);
        auto* lab = qobject_cast<QLabel*>(m_transcript->itemWidget(it));
        it->setHidden(!text.isEmpty() && lab && !lab->text().contains(text, Qt::CaseInsensitive));
    }
}

// ================= 历史/播放列表 =================
void MainWindow::loadHistory() {
    m_historyList->clear();
    const QStringList hist = m_settings.value(QStringLiteral("history/paths")).toStringList();
    for (const QString& p : hist) {
        // 绝不同步 QFile::exists()：历史可能含网盘挂载路径（如 X:/），离线时阻塞 UI（启动卡死实测）。
        auto* it = new QListWidgetItem(m_historyList);   // 文字留空：条目自带文字会被
        it->setToolTip(p);                               // 默认委托画在透明行控件底下（重影）
        it->setData(Qt::UserRole, p);
    }
}

void MainWindow::saveHistory(const QString& path) {
    QStringList hist = m_settings.value(QStringLiteral("history/paths")).toStringList();
    hist.removeAll(path);
    hist.prepend(path);
    while (hist.size() > 30) hist.removeLast();
    m_settings.setValue(QStringLiteral("history/paths"), hist);
    rcpMark("ctor:loadHistory-begin");
    loadHistory();
    rcpMark("ctor:loadHistory-done");
}

void MainWindow::addMediaPaths(const QStringList& paths) {
    for (const QString& p : paths) {
        if (m_mediaPaths.contains(p)) continue;
        m_mediaPaths.append(p);
        auto* it = new QListWidgetItem(m_mediaList);
        it->setToolTip(p);
        it->setData(Qt::UserRole, p);
    }
    if (!m_mediaPaths.isEmpty() && m_mediaList->currentRow() < 0) m_mediaList->setCurrentRow(0);
    refreshMediaRows();
}

// 播放列表/历史行：18px 播放位 + 名称(12px) + 时长(11px)，当前项 ▶ + 白字
void MainWindow::refreshMediaRows() {
    auto decorate = [this](QListWidget* list) {
        for (int i = 0; i < list->count(); ++i) {
            QListWidgetItem* it = list->item(i);
            const QString p = it->data(Qt::UserRole).toString();
            QWidget* w = list->itemWidget(it);
            if (!w) {
                w = new QWidget;
                w->setAttribute(Qt::WA_TransparentForMouseEvents);
                w->setStyleSheet(QStringLiteral("background:transparent;"));
                auto* h = new QHBoxLayout(w);
                h->setContentsMargins(0, 8, 0, 8);
                h->setSpacing(7);
                auto* play = new QLabel(w);
                play->setObjectName(QStringLiteral("rowPlay"));
                auto* name = new QLabel(w);
                name->setObjectName(QStringLiteral("rowName"));
                auto* dur = new QLabel(w);
                dur->setObjectName(QStringLiteral("rowDur"));
                h->addWidget(play);
                h->addWidget(name, 1);
                h->addWidget(dur);
                it->setSizeHint(QSize(218, 40));
                list->setItemWidget(it, w);
            }
            const bool current = (p == m_currentPath);
            const bool selected = (list->currentItem() == it);
            // 参考 .playlist-item.active：选中项紫色渐变胶囊铺满行
            w->setStyleSheet(current || selected
                ? QStringLiteral("background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
                                 "stop:0 #5147aa, stop:1 #6254cb);border-radius:8px;")
                : QStringLiteral("background:transparent;"));
            const QString fg = (current || selected) ? QStringLiteral("#ffffff")
                                                     : QStringLiteral("#a7adb8");
            auto* play = w->findChild<QLabel*>(QStringLiteral("rowPlay"));
            auto* name = w->findChild<QLabel*>(QStringLiteral("rowName"));
            auto* dur = w->findChild<QLabel*>(QStringLiteral("rowDur"));
            play->setText(current ? QStringLiteral("\u25B6") : QString());
            play->setStyleSheet(
                QStringLiteral("color:%1;font-size:9px;background:transparent;").arg(fg));
            QFont f12 = name->font();
            f12.setPixelSize(12);
            const QFontMetrics fm12(f12);
            name->setFixedWidth(110);
            name->setText(fm12.elidedText(QFileInfo(p).fileName(), Qt::ElideRight, 110));
            name->setStyleSheet(
                QStringLiteral("color:%1;font-size:12px;background:transparent;").arg(fg));
            const qint64 d = m_demoDurations.value(QFileInfo(p).fileName(),
                m_settings.value(QStringLiteral("duration/") + QFileInfo(p).fileName(), 0).toLongLong());
            dur->setText(d > 999 ? formatTime(d / 1000.0) : QStringLiteral("--:--"));
            dur->setStyleSheet(QStringLiteral(
                "color:rgba(255,255,255,0.78);font-size:11px;background:transparent;%1")
                                   .arg(current || selected ? QString() : QStringLiteral("color:#a7adb8;")));
        }
    };
    decorate(m_mediaList);
    decorate(m_historyList);
}

void MainWindow::onPlaylistActivated(QListWidgetItem* item) {
    if (!item || m_demoMode) return;   // 示例数据为展示态，不触发真实加载
    const QString p = item->data(Qt::UserRole).toString();
    if (!p.isEmpty()) openFile(p);
}

// ================= 打开/播放 =================
void MainWindow::openFile(const QString& path) {
    if (!m_player || !m_player->handle()) return;
    if (m_demoMode) {
        // 首个真实媒体：整体退出示例数据态（列表/转写/统计由真实数据重建）。
        m_demoMode = false;
        if (m_demoTimer) m_demoTimer->stop();
        m_mediaPaths.clear();
        m_mediaList->clear();
        m_historyList->clear();
        m_demoDurations.clear();
        m_partialText.clear();
        m_overlayFinalText.clear();
    }
    rcpTrace(QStringLiteral("openFile -> %1").arg(path));
    m_currentPath = path;   // 先于 loadFile：durationChanged 可能先到
    if (m_player->loadFile(path)) {
        addMediaPaths(QStringList{path});
        saveHistory(path);
        const int row = m_mediaPaths.indexOf(path);
        if (row >= 0) m_mediaList->setCurrentRow(row);
        refreshMediaRows();
        const QString name = QFileInfo(path).fileName();
        m_titleFile->setText(name);
        // 画面内标题去扩展名（参考 .video-file-title stripExtension），标题栏保留原名
        m_videoFileTitle->setText(QFileInfo(path).completeBaseName());
        setWindowTitle(QStringLiteral("实时字幕播放器 — %1").arg(name));
        if (m_settings.value(QStringLiteral("general/rememberPosition"), true).toBool()) {
            const qint64 saved = m_settings.value(QStringLiteral("position/") + name, 0).toLongLong();
            if (saved > 5000 && saved < m_duration * 1000.0 - 2000)   // 接近片尾不恢复（避免立即 EOF）
                QTimer::singleShot(300, this, [this, saved] { m_player->seek(saved / 1000.0, false); });
        }
        if (m_settings.value(QStringLiteral("general/autoPlay"), true).toBool()) m_player->play();
        m_waveform->pulse(0.8);
        startCaptioningFor(path);
        repositionOverlays();
    }
}

// 启动字幕 worker（诊断开关 RCP_NO_CAPTION=1 可跳过）。
void MainWindow::startCaptioningFor(const QString& path) {
    if (!qEnvironmentVariableIsEmpty("RCP_NO_CAPTION")) return;
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString workerExe = appDir + QStringLiteral("/caption_worker.exe");
    // 打包布局（bundle/MSI）模型在 <appDir>/models；开发机布局在 <appDir>/.tools/models。
    QString modelsRoot = appDir + QStringLiteral("/models");
    if (!QFileInfo::exists(modelsRoot))
        modelsRoot = appDir + QStringLiteral("/.tools/models");

    if (m_worker->isRunning()) m_worker->shutdown();
    m_transcript->clear();
    m_lastPartialLabel = nullptr;
    m_finalCount = 0;
    m_statLines->setText(tr("字幕行数：0"));

    if (!QFile::exists(workerExe)) {
        setAsrStatus(tr("缺少 caption_worker"), QStringLiteral("#ff6b79"));
        return;
    }
    m_worker->start(workerExe, QFileInfo(path).absoluteFilePath(), modelsRoot);
}

void MainWindow::onOpen() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("打开媒体文件"), QString(),
        tr("媒体文件 (*.mp4 *.mkv *.avi *.mov *.webm *.mp3 *.mka *.wav);;所有文件 (*.*)"));
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::onOpenFolder() {
    const QString dir = QFileDialog::getExistingDirectory(this, tr("打开包含视频的文件夹"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    if (dir.isEmpty()) return;
    QStringList found;
    for (const QFileInfo& fi : QDir(dir).entryInfoList(
             QStringList{"*.mp4", "*.mkv", "*.avi", "*.mov", "*.webm"}, QDir::Files)) {
        found << fi.absoluteFilePath();
    }
    if (found.isEmpty()) return;
    addMediaPaths(found);
    openFile(found.first());
}

void MainWindow::onPlayPause() { m_player->togglePause(); }

void MainWindow::onPrevNext(int delta) {
    if (m_mediaPaths.isEmpty()) return;
    const int row = qBound(0, m_mediaList->currentRow() + delta, m_mediaPaths.size() - 1);
    m_mediaList->setCurrentRow(row);
    openFile(m_mediaPaths.at(row));
}

void MainWindow::onSpeed(double v) {
    if (m_demoMode) return;   // 示例态固定 1.50x（ctor 的速度恢复 singleShot 不得覆盖）
    m_player->setSpeed(v);
    m_btnSpeed->setText(QString::number(v, 'f', 2) + QStringLiteral("x ⌄"));
    m_settings.setValue(QStringLiteral("playback/speed"), v);
}

void MainWindow::onVolumeChanged(int value) {
    m_player->setVolume(value);
    m_settings.setValue(QStringLiteral("playback/volume"), value);
}

void MainWindow::onScreenshot() {
    if (m_player->handle()) {
        const char* args[] = {"screenshot", nullptr};
        mpv_command(m_player->handle(), args);
    }
}

void MainWindow::onAbLoop() {
    const double pos = m_player->timePosition();
    if (m_abClicks == 0) {
        double a = pos;
        mpv_set_property(m_player->handle(), "ab-loop-a", MPV_FORMAT_DOUBLE, &a);
        m_btnAb->setText(QStringLiteral("A"));
    } else if (m_abClicks == 1) {
        double b = pos;
        mpv_set_property(m_player->handle(), "ab-loop-b", MPV_FORMAT_DOUBLE, &b);
        m_btnAb->setText(QStringLiteral("AB"));
    } else {
        mpv_set_property_string(m_player->handle(), "ab-loop-a", "no");
        mpv_set_property_string(m_player->handle(), "ab-loop-b", "no");
        m_btnAb->setText(QStringLiteral("AB"));
    }
    m_abClicks = (m_abClicks + 1) % 3;
}

void MainWindow::onCycleSub() {
    if (m_player->handle()) {
        const char* args[] = {"cycle", "sub", nullptr};
        mpv_command(m_player->handle(), args);
    }
}

// ================= 进度/时长 =================
void MainWindow::onPositionChanged(double seconds) {
    if (m_seeking) return;
    if (m_duration > 0.0) m_seek->setValue(static_cast<int>(seconds));
    m_timeCur->setText(formatTime(seconds));
    updateOverlay();
    const long long head = static_cast<long long>(seconds * 1000.0);
    if (m_lastFinalEndMs > 0) {
        const double lead = qMax<long long>(0, m_lastFinalEndMs - head) / 1000.0;
        m_statLatency->setText(tr("实时延迟：%1s").arg(lead, 0, 'f', 1));
        m_asrLatency->setText(tr("%1s").arg(lead, 0, 'f', 1));
    }
    m_asrRecognized->setText(formatTime(m_lastFinalEndMs / 1000.0));
    static qint64 lastSave = 0;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastSave > 4000 && m_duration > 0) {
        lastSave = now;
        m_settings.setValue(QStringLiteral("position/") + m_titleFile->text(),
                            static_cast<qint64>(seconds * 1000.0));
    }
}

void MainWindow::onDurationChanged(double seconds) {
    m_duration = seconds;
    if (seconds > 0.0) {
        m_seek->setRange(0, static_cast<int>(seconds));
        if (!m_currentPath.isEmpty()) {
            m_settings.setValue(QStringLiteral("duration/") + QFileInfo(m_currentPath).fileName(),
                                static_cast<qint64>(seconds));
        }
    }
    m_timeDur->setText(formatTime(seconds));
    refreshMediaRows();
}

void MainWindow::onSeekSlider(int value) {
    m_seeking = true;
    m_timeCur->setText(formatTime(value));
}

void MainWindow::onSeekReleased() {
    m_player->seek(m_seek->value(), false);
    m_seeking = false;
}

void MainWindow::onMediaLoaded() {
    m_playBtn->setIcon(icon(QStringLiteral("pause"), QColor("#ffffff"), 22));
    repositionOverlays();   // video-params 就绪：字幕/徽标锚定画面矩形
}

void MainWindow::onMediaEnded() {
    m_playBtn->setIcon(icon(QStringLiteral("play"), QColor("#ffffff"), 22));
    m_worker->stop();
    // 播完清除记忆位置：否则下次打开自动 seek 到片尾 → 立即 EOF → 黑屏死循环。
    m_settings.remove(QStringLiteral("position/") + m_titleFile->text());
    if (m_settings.value(QStringLiteral("playback/folderContinue"), true).toBool()
        && m_mediaPaths.size() > 1) {
        onPrevNext(1);
        return;
    }
    if (m_settings.value(QStringLiteral("caption/autoExport"), false).toBool() && !m_finals.isEmpty()) {
        const QString path = m_mediaPaths.value(qMax(0, m_mediaList->currentRow()));
        if (!path.isEmpty()) {
            QList<rcp::CaptionSegment> list;
            for (const auto& s : m_finals) list.append(s);
            rcp::captions::SrtExporter::writeSrt(path + QStringLiteral(".srt"), list);
        }
    }
}

// ================= 字幕开关/导出 =================
void MainWindow::onExportSrt() {
    const QString path = QFileDialog::getSaveFileName(this, tr("导出字幕 SRT"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
                                                      tr("SubRip (*.srt)"));
    if (path.isEmpty()) return;
    QList<rcp::CaptionSegment> list;
    for (const auto& s : m_captionCtl->finals()) list.append(s);
    const auto res = rcp::captions::SrtExporter::writeSrt(path, list);
    if (res.isError()) appendTranscriptFinal(-1, tr("[导出失败] %1").arg(path));
    else appendTranscriptFinal(-1, tr("[已导出] %1").arg(path));
}

void MainWindow::onToggleCaption() {
    m_captionOn = m_btnCaption->isChecked();
    if (!m_captionOn) {
        m_partialLabel->clear();
        m_finalLabel->clear();
    } else {
        updateOverlay();
    }
}

void MainWindow::onToggleSettings() {
    m_settingsPanel->setVisible(!m_settingsPanel->isVisible());
}

void MainWindow::onToggleFullscreen() { isFullScreen() ? showNormal() : showFullScreen(); }

// ================= 字幕叠加/样式 =================
void MainWindow::updateOverlay() {
    if (!m_captionOn) return;
    if (!qEnvironmentVariableIsEmpty("RCP_NO_OVERLAY")) return;   // 诊断开关
    const bool showPartial = m_settings.value(QStringLiteral("caption/showPartial"), true).toBool();
    // 参考稿视觉：叠加层只保留"当前句尾"——partial 一行、final 至多两行。
    // worker 的 partial 覆盖整段未完句（授课式语音一段可达数百字），直接
    // 显示会换行成文字墙。按像素宽度取尾部，截断处加省略号。
    auto tailToFit = [](const QString& t, const QFont& f, qreal maxW, int maxLines) {
        if (t.isEmpty()) return t;
        const QFontMetrics fm(f);
        // 用 CJK 字宽做单位（averageCharWidth 被 Latin 拉低 → 截断失效换行成墙）
        const qreal unit = qMax<qreal>(4.0, fm.horizontalAdvance(QChar(0x6D4B)));
        const int perLine = qMax(6, static_cast<int>(maxW / unit));
        const int maxChars = perLine * qMax(1, maxLines);
        if (t.size() <= maxChars) return t;
        return QStringLiteral("…") + t.right(maxChars - 1);
    };
    const QString prevPartial = m_partialLabel->text();
    const QString prevFinal = m_finalLabel->text();
    const qreal pw = qMax<qreal>(320.0, m_partialLabel->width() - 8.0);
    const qreal fw = qMax<qreal>(320.0, m_finalLabel->width() - 8.0);
    m_partialLabel->setText(showPartial ? tailToFit(m_partialText, m_partialLabel->font(), pw, 1) : QString());
    m_partialLabel->setVisible(showPartial && !m_partialText.isEmpty());
    // 定稿显示策略（双兜底）：
    // 1) worker 离线提取音频通常跑赢播放头——只显示播放头已到达的句子
    //    （startMs <= head+0.5s 的最后一条），字幕与语音同步；
    // 2) worker 落后/seek/恢复会话时可能没有任何已到达句——退回常驻显示
    //    最后到达的一句，不空窗（用户报"直接看不到字幕"的根因即定稿空窗）。
    // 两种情况都不要求 head <= 句尾：句子显示到下一句开始（参考稿行为）。
    const double s = m_player->timePosition() + m_delayMs / 1000.0;
    const long long head = static_cast<long long>(s * 1000.0) + 500;
    QString activeFinal;
    for (const auto& seg : m_finals) {
        if (seg.startMs <= head) activeFinal = seg.text;
    }
    if (activeFinal.isEmpty()) activeFinal = m_overlayFinalText;
    // 参考稿 final 恒为单行短句：超宽取尾加省略号（2 行会顶压 partial 行）。
    const QString shownFinal = tailToFit(activeFinal, m_finalLabel->font(), fw, 1);
    if (m_finalLabel->text() != shownFinal) m_finalLabel->setText(shownFinal);
    // 叠加文字变化时强制视频区整体重组：QOpenGLWidget 的子控件脏区合成
    // 在仅子控件重绘时可能残留下帧旧文字（重影），整块 update 一并消除。
    if (m_video && (m_partialLabel->text() != prevPartial || m_finalLabel->text() != prevFinal))
        m_video->update();
}

void MainWindow::applyCaptionStyle() {
    const int size = m_settings.value(QStringLiteral("caption/size"), 36).toInt();
    m_finalLabel->setFontSizePx(size);
    m_partialLabel->setFontSizePx(qMax(12, static_cast<int>(size * 0.82)));
    m_partialLabel->setCaptionColor(QColor(225, 228, 234, 171));
    // 字幕字体（参考设置页：思源黑体/微软雅黑/系统默认；setFontSizePx 会重置
    // 字体族，故在字号之后应用）
    QStringList fam;
    switch (m_settings.value(QStringLiteral("caption/font"), 0).toInt()) {
    case 0: fam = {QStringLiteral("Source Han Sans SC"), QStringLiteral("思源黑体"),
                   QStringLiteral("Noto Sans CJK SC"), QStringLiteral("Microsoft YaHei UI")}; break;
    case 1: fam = {QStringLiteral("Microsoft YaHei"), QStringLiteral("Microsoft YaHei UI")}; break;
    default: fam = {QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei UI")}; break;
    }
    for (rcpui::CaptionLabel* l : {m_finalLabel, m_partialLabel}) {
        QFont f = l->font();
        f.setFamilies(fam);
        l->setFont(f);
    }
    // 字幕颜色（参考：黄=#ffe66d）；描边/阴影模式（0描边 1阴影 2关闭）
    const bool yellow = m_settings.value(QStringLiteral("caption/color"), 0).toInt() == 1;
    m_finalLabel->setCaptionColor(yellow ? QColor(0xff, 0xe6, 0x6d) : QColor("#ffffff"));
    m_finalLabel->setOutlineMode(m_settings.value(QStringLiteral("caption/outline"), 0).toInt());
}

void MainWindow::repositionOverlays() {
    QWidget* host = m_video ? qobject_cast<QWidget*>(m_video) : m_videoHost;
    if (!host || host->width() <= 0) return;
    const int vw = host->width(), vh = host->height();
    m_vignette->setGeometry(0, 0, vw, vh);
    m_dropHint->setGeometry(18, 18, vw - 36, vh - 36);
    // 参考稿 --caption-bottom: 13% 相对**视频画面**：mpv 保持纵横比 letterbox 时
    // 按 contain-fit 求画面矩形，字幕/波形/徽标都锚定画面而非整个控件
    // （否则宽高比不匹配时字幕落在黑边里，偏离参考效果）。
    QRect c(0, 0, vw, vh);
    const QSize vs = m_player ? m_player->videoSize() : QSize();
    m_videoRectValid = vs.isValid();   // mediaLoaded 时 video-params 常未就绪：播放头事件里重试
    if (vs.isValid()) {
        const double scale = qMin(double(vw) / vs.width(), double(vh) / vs.height());
        const int cw = qMax(1, qRound(vs.width() * scale));
        const int ch = qMax(1, qRound(vs.height() * scale));
        c = QRect((vw - cw) / 2, (vh - ch) / 2, cw, ch);
    }
    const int ow = qMin(static_cast<int>(c.width() * 0.86), 920);
    const int oh = qMin(c.height() / 2, 180);
    const int pos = m_settings.value(QStringLiteral("caption/position"), 0).toInt();
    int oy = c.y() + static_cast<int>(c.height() * 0.87) - oh;
    if (1 == pos) oy = c.y() + (c.height() - oh) / 2;
    if (2 == pos) oy = c.y() + static_cast<int>(c.height() * 0.10);
    m_captionOverlay->setGeometry(c.x() + (c.width() - ow) / 2, qMax(c.y(), oy), ow, oh);
    rcpTrace(QStringLiteral("reposition host=%1x%2 video=%3x%4 content=(%5,%6 %7x%8) box=(%9,%10 %11x%12)")
                 .arg(vw).arg(vh).arg(vs.width()).arg(vs.height())
                 .arg(c.x()).arg(c.y()).arg(c.width()).arg(c.height())
                 .arg(m_captionOverlay->x()).arg(m_captionOverlay->y()).arg(ow).arg(oh));
    m_waveform->setGeometry(c.x() + (c.width() - qMin(c.width() * 48 / 100, 520)) / 2,
                            qMin(c.bottom() - 16, oy + oh + 17), qMin(c.width() * 48 / 100, 520), 14);
    // 隐私徽标：画面内 top 17 right 18（参考稿 privacy-badge）
    if (m_privacyBadge) {
        m_privacyBadge->adjustSize();
        m_privacyBadge->move(c.right() - m_privacyBadge->width() - 18, c.y() + 17);
    }
}

// ================= 事件 =================
bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton)
                m_windowDrag = me->globalPosition().toPoint() - frameGeometry().topLeft();
        } else if (event->type() == QEvent::MouseMove) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->buttons() & Qt::LeftButton && !isMaximized())
                move(me->globalPosition().toPoint() - m_windowDrag);
        }
    } else if (obj == m_videoHost && event->type() == QEvent::Resize) {
        repositionOverlays();
    } else if (obj == m_videoHost && event->type() == QEvent::Enter) {
        if (!m_currentPath.isEmpty() && m_privacyBadge) m_privacyBadge->show();   // 悬停显示（参考稿 hover 态）
    } else if (obj == m_videoHost && event->type() == QEvent::Leave) {
        if (m_privacyBadge) m_privacyBadge->hide();
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        if (m_dropHint) m_dropHint->show();
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent*) {
    if (m_dropHint) m_dropHint->hide();
}

void MainWindow::dropEvent(QDropEvent* event) {
    if (m_dropHint) m_dropHint->hide();
    QStringList files;
    for (const QUrl& u : event->mimeData()->urls())
        if (u.isLocalFile()) files << u.toLocalFile();
    if (files.isEmpty()) return;
    addMediaPaths(files);
    openFile(files.first());
}

void MainWindow::closeEvent(QCloseEvent*) {}

QString MainWindow::formatTime(double seconds) const {
    if (seconds < 0) seconds = 0;
    const int total = static_cast<int>(seconds);
    return QStringLiteral("%1:%2").arg(total / 60, 2, 10, QChar('0')).arg(total % 60, 2, 10, QChar('0'));
}
