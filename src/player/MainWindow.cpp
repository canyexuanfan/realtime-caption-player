// src/player/MainWindow.cpp
// UI 按 reference/前端参考 05_Single_HTML_Frontend_Reference.html 复刻：
// 暗色主题（bg #090b0f / panel #15191f / accent #7868ff）、左侧 238px 栏
// （播放列表 + 实时字幕转写）、中央视频 + 浮动状态卡、底部控制台。
// 仅实现 MVP 已有的真实能力，不放假按钮（截图/AB 循环/设置页待对应任务）。
#include "MainWindow.h"
#include "MpvPlayer.h"
#include "MpvRenderWidget.h"
#include "WorkerSupervisor.h"
#include "captions/CaptionController.h"
#include "captions/SrtExporter.h"
#include "captions/CaptionTypes.h"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QPushButton>
#include <QShortcut>
#include <QSlider>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// 参考设计的设计变量（:root 设计令牌）。
const char* kAppQss = R"(
QWidget { background: #090b0f; color: #f4f6fb; font-family: "Segoe UI", "Microsoft YaHei UI"; font-size: 13px; }
QMainWindow { background: #090b0f; }

/* ---- 自绘标题栏 ---- */
QWidget#titleBar { background: #111419; border-bottom: 1px solid rgba(255,255,255,0.105); }
QLabel#brand { color: #f4f6fb; font-size: 13px; font-weight: 600; background: transparent; }
QLabel#brandDot { color: #7868ff; font-size: 15px; background: transparent; }
QLabel#titleFile { color: #a7adb8; background: transparent; }
QPushButton.titleAction {
    background: #20252d; color: #e6e9f0; border: 1px solid rgba(255,255,255,0.105);
    border-radius: 6px; padding: 5px 12px;
}
QPushButton.titleAction:hover { background: #222832; border-color: rgba(255,255,255,0.16); }
QPushButton#btnOpenFile { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #5147aa, stop:1 #6254cb);
    border: none; color: white; font-weight: 600; }
QPushButton#btnOpenFile:hover { background: #6254cb; }
QPushButton.winAction { background: transparent; border: none; border-radius: 6px; color: #a7adb8; font-size: 14px; padding: 4px 10px; }
QPushButton.winAction:hover { background: #222832; color: #f4f6fb; }
QPushButton#btnClose:hover { background: #ff6b79; color: #14161a; }

/* ---- 左侧栏 ---- */
QFrame#leftRail { background: #15191f; border: 1px solid rgba(255,255,255,0.105); border-radius: 10px; }
QLabel.railTitle { color: #a7adb8; font-size: 12px; font-weight: 600; background: transparent; }
QPushButton.iconBtn { background: transparent; border: 1px solid rgba(255,255,255,0.105); border-radius: 6px;
    color: #a7adb8; padding: 2px 8px; font-size: 14px; }
QPushButton.iconBtn:hover { background: #222832; color: #f4f6fb; }
QListWidget { background: #191e25; border: 1px solid rgba(255,255,255,0.105); border-radius: 8px;
    outline: none; padding: 4px; }
QListWidget::item { color: #e6e9f0; border-radius: 6px; padding: 7px 9px; margin: 1px 0; }
QListWidget::item:hover { background: #222832; }
QListWidget::item:selected { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #5147aa, stop:1 #6254cb); color: white; }
QLabel#stats { color: #757c88; font-size: 11px; background: transparent; }

/* ---- 视频区浮层 ---- */
QLabel#privacyBadge { background: rgba(21,25,31,0.82); color: #61d27d; border: 1px solid rgba(97,210,125,0.35);
    border-radius: 999px; padding: 4px 12px; font-size: 11px; }
QFrame#asrCard { background: rgba(21,25,31,0.94); border: 1px solid rgba(255,255,255,0.16); border-radius: 10px; }
QFrame#asrCard QLabel { background: transparent; }
QLabel#asrHead { color: #f4f6fb; font-weight: 600; font-size: 12px; }
QLabel.asrKey { color: #757c88; font-size: 11px; }
QLabel.asrVal { color: #cdd2db; font-size: 11px; }

/* ---- 控制台 ---- */
QWidget#controlDeck { background: #15191f; border: 1px solid rgba(255,255,255,0.105); border-radius: 10px; }
QWidget#controlDeck QLabel { background: transparent; color: #a7adb8; font-size: 12px; }
QPushButton.ctrlBtn { background: #20252d; border: 1px solid rgba(255,255,255,0.105); border-radius: 8px;
    color: #e6e9f0; padding: 8px 14px; font-size: 14px; }
QPushButton.ctrlBtn:hover { background: #222832; border-color: rgba(255,255,255,0.16); }
QPushButton#btnPlay { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #5147aa, stop:1 #6254cb);
    border: none; color: white; font-size: 16px; padding: 9px 22px; font-weight: 600; }
QPushButton#btnPlay:hover { background: #6254cb; }
QPushButton#btnCaption:checked { border-color: #7868ff; color: white; background: rgba(120,104,255,0.18); }

QSlider::groove:horizontal { border: none; height: 5px; border-radius: 2px; background: #222832; }
QSlider::sub-page:horizontal { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #5147aa, stop:1 #7868ff); border-radius: 2px; }
QSlider::handle:horizontal { width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; background: #f4f6fb; }
QSlider::handle:horizontal:hover { background: #ffffff; }

QComboBox { background: #20252d; color: #e6e9f0; border: 1px solid rgba(255,255,255,0.105); border-radius: 8px; padding: 6px 12px; }
QComboBox:hover { background: #222832; }
QComboBox QAbstractItemView { background: #191e25; color: #e6e9f0; selection-background-color: #5c4bd5; border: 1px solid rgba(255,255,255,0.16); }
QScrollBar:vertical { background: transparent; width: 8px; margin: 2px; }
QScrollBar::handle:vertical { background: #222832; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #2c333e; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)";
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_player = new MpvPlayer(this);

    if (!m_player->create()) {
        setAsrStatus(tr("libmpv 初始化失败"), QStringLiteral("#ff6b79"));
    } else {
        m_player->setOption(QStringLiteral("vo"), QStringLiteral("libmpv"));
        m_player->setOption(QStringLiteral("hwdec"), QStringLiteral("no"));
        m_player->initialize();
    }

    m_captionCtl = new rcp::captions::CaptionController(this);
    m_worker = new rcp::player::WorkerSupervisor(this);

    applyTheme();
    setupUi();

    // 播放内核信号
    connect(m_player, &MpvPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &MpvPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player, &MpvPlayer::mediaLoaded, this, &MainWindow::onMediaLoaded);
    connect(m_player, &MpvPlayer::mediaEnded, this, &MainWindow::onMediaEnded);
    connect(m_player, &MpvPlayer::pauseStateChanged, this, [this](bool paused) {
        m_playPauseBtn->setText(paused ? QStringLiteral("\u25B6") : QStringLiteral("\u23F8"));
    });

    // ---- 字幕链路：worker 事件 -> 控制器 -> mpv 叠加层 + 转写面板 ----
    connect(m_worker, &rcp::player::WorkerSupervisor::sessionStarted,
            m_captionCtl, &rcp::captions::CaptionController::reset);
    connect(m_worker, &rcp::player::WorkerSupervisor::sessionStarted, this, [this] {
        setAsrStatus(tr("识别中"), QStringLiteral("#7868ff"));
        m_finalCount = 0;
        m_stats->setText(tr("字幕行数：0"));
    });
    connect(m_worker, &rcp::player::WorkerSupervisor::captionSegment,
            this, [this](const rcp::CaptionSegment& seg, bool isPartial) {
                if (isPartial) {
                    m_captionCtl->onPartial(seg, seg.generation);
                    appendTranscriptPartial(seg.text);
                } else {
                    m_captionCtl->onFinal(seg, seg.generation);
                    appendTranscriptFinal(seg.text, seg.startMs);
                }
            });
    connect(m_worker, &rcp::player::WorkerSupervisor::ready, this, [this] {
        setAsrStatus(tr("运行中"), QStringLiteral("#61d27d"));
    });
    connect(m_worker, &rcp::player::WorkerSupervisor::workerError, this,
            [this](const QString& m) {
                setAsrStatus(tr("字幕错误"), QStringLiteral("#ff6b79"));
                appendTranscriptFinal(tr("[字幕错误] %1").arg(m), -1);
            });
    connect(m_worker, &rcp::player::WorkerSupervisor::workerFinished, this, [this] {
        setAsrStatus(tr("识别进程退出"), QStringLiteral("#757c88"));
    });
    connect(m_captionCtl, &rcp::captions::CaptionController::overlayChanged, this,
            [this](const QString& ass) {
                if (m_captionOn && !ass.isEmpty())
                    m_player->showSubtitleOverlay(ass);
                else
                    m_player->clearSubtitleOverlay();
            });
    // 播放头驱动叠加层对齐
    connect(m_player, &MpvPlayer::positionChanged, m_captionCtl,
            [this](double s) {
                m_captionCtl->setPlayheadMs(static_cast<long long>(s * 1000.0));
            });

    setAcceptDrops(true);
    // 自绘标题栏：无边框窗口（参考原型的 app-window 视觉）。
    setWindowFlag(Qt::FramelessWindowHint, true);
    resize(1280, 800);
    setMinimumSize(1024, 660);
}

void MainWindow::applyTheme() {
    qApp->setStyleSheet(QString::fromUtf8(kAppQss));
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(buildTitleBar(central));

    auto* body = new QHBoxLayout();
    body->setContentsMargins(8, 8, 8, 8);
    body->setSpacing(8);
    body->addWidget(buildLeftRail(central));
    body->addWidget(buildStage(central), 1);
    root->addLayout(body, 1);

    setCentralWidget(central);

    // ---- 键盘快捷键（参考：空格播放/暂停，←→ ±10s，↑↓ 音量，F 全屏，Ctrl+O 打开）----
    auto sc = [this](const char* key, auto slot) {
        auto* s = new QShortcut(QKeySequence(QLatin1String(key)), this);
        connect(s, &QShortcut::activated, this, slot);
    };
    sc("Space", &MainWindow::onPlayPause);
    sc("Left",  [this] { m_player->seek(qBound(0.0, m_player->timePosition() - 10.0, m_duration), false); });
    sc("Right", [this] { m_player->seek(qBound(0.0, m_player->timePosition() + 10.0, m_duration), false); });
    sc("Up",    [this] { m_volume->setValue(qMin(100, m_volume->value() + 5)); });
    sc("Down",  [this] { m_volume->setValue(qMax(0, m_volume->value() - 5)); });
    sc("F",     &MainWindow::onToggleFullscreen);
    sc("Ctrl+O", &MainWindow::onOpen);
}

QWidget* MainWindow::buildTitleBar(QWidget* parent) {
    m_titleBar = new QWidget(parent);
    m_titleBar->setObjectName(QStringLiteral("titleBar"));
    m_titleBar->setFixedHeight(48);
    m_titleBar->installEventFilter(this);   // 自绘标题栏拖拽移动

    auto* h = new QHBoxLayout(m_titleBar);
    h->setContentsMargins(16, 0, 10, 0);
    h->setSpacing(8);

    auto* dot = new QLabel(QStringLiteral("\u25CF"), m_titleBar);
    dot->setObjectName(QStringLiteral("brandDot"));
    auto* brand = new QLabel(tr("实时字幕播放器"), m_titleBar);
    brand->setObjectName(QStringLiteral("brand"));

    m_titleFile = new QLabel(tr("未打开媒体"), m_titleBar);
    m_titleFile->setObjectName(QStringLiteral("titleFile"));

    auto* btnOpen = new QPushButton(tr("打开文件"), m_titleBar);
    btnOpen->setObjectName(QStringLiteral("btnOpenFile"));
    btnOpen->setCursor(Qt::PointingHandCursor);
    connect(btnOpen, &QPushButton::clicked, this, &MainWindow::onOpen);

    m_btnMin = new QPushButton(QStringLiteral("\u2013"), m_titleBar);
    m_btnMax = new QPushButton(QStringLiteral("\u25A1"), m_titleBar);
    m_btnClose = new QPushButton(QStringLiteral("\u2715"), m_titleBar);
    for (auto* b : {m_btnMin, m_btnMax, m_btnClose}) {
        b->setProperty("class", "winAction");
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(34, 28);
    }
    m_btnClose->setObjectName(QStringLiteral("btnClose"));
    connect(m_btnMin, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(m_btnMax, &QPushButton::clicked, this, [this] {
        isFullScreen() ? showNormal() : showMaximized();
    });
    connect(m_btnClose, &QPushButton::clicked, this, &QWidget::close);

    h->addWidget(dot);
    h->addWidget(brand);
    h->addSpacing(12);
    h->addWidget(m_titleFile, 1);
    h->addWidget(btnOpen);
    h->addSpacing(6);
    h->addWidget(m_btnMin);
    h->addWidget(m_btnMax);
    h->addWidget(m_btnClose);
    return m_titleBar;
}

QWidget* MainWindow::buildLeftRail(QWidget* parent) {
    auto* rail = new QFrame(parent);
    rail->setObjectName(QStringLiteral("leftRail"));
    rail->setFixedWidth(238);

    auto* v = new QVBoxLayout(rail);
    v->setContentsMargins(10, 10, 10, 10);
    v->setSpacing(8);

    // 播放列表
    auto* plHead = new QHBoxLayout();
    auto* plTitle = new QLabel(tr("播放列表"), rail);
    plTitle->setProperty("class", "railTitle");
    auto* btnAdd = new QPushButton(QStringLiteral("+"), rail);
    btnAdd->setProperty("class", "iconBtn");
    btnAdd->setToolTip(tr("添加视频"));
    btnAdd->setCursor(Qt::PointingHandCursor);
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::onOpen);
    plHead->addWidget(plTitle);
    plHead->addStretch();
    plHead->addWidget(btnAdd);
    v->addLayout(plHead);

    m_playlist = new QListWidget(rail);
    m_playlist->setContextMenuPolicy(Qt::NoContextMenu);
    connect(m_playlist, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        onPlaylistActivated(m_playlist->currentRow());
    });
    v->addWidget(m_playlist, 5);

    // 实时字幕转写面板
    auto* trHead = new QHBoxLayout();
    auto* trTitle = new QLabel(tr("实时字幕"), rail);
    trTitle->setProperty("class", "railTitle");
    auto* btnClear = new QPushButton(tr("清空"), rail);
    btnClear->setProperty("class", "iconBtn");
    btnClear->setCursor(Qt::PointingHandCursor);
    connect(btnClear, &QPushButton::clicked, this, [this] {
        m_transcript->clear();
        m_finalCount = 0;
        m_stats->setText(tr("字幕行数：0"));
    });
    trHead->addWidget(trTitle);
    trHead->addStretch();
    trHead->addWidget(btnClear);
    v->addLayout(trHead);

    m_transcript = new QListWidget(rail);
    m_transcript->setSelectionMode(QAbstractItemView::NoSelection);
    v->addWidget(m_transcript, 6);

    m_stats = new QLabel(tr("字幕行数：0"), rail);
    m_stats->setObjectName(QStringLiteral("stats"));
    v->addWidget(m_stats);
    return rail;
}

QWidget* MainWindow::buildStage(QWidget* parent) {
    auto* stage = new QWidget(parent);
    auto* v = new QVBoxLayout(stage);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(8);

    // 视频容器：mpv 渲染 + 角标浮层（同一网格单元叠加）
    auto* container = new QFrame(stage);
    container->setStyleSheet(QStringLiteral(
        "QFrame { background: #000000; border: 1px solid rgba(255,255,255,0.105); border-radius: 10px; }"));
    auto* grid = new QGridLayout(container);
    grid->setContentsMargins(0, 0, 0, 0);

    m_video = new MpvRenderWidget(container);
    m_video->setMinimumSize(320, 240);
    if (m_player && m_player->handle()) m_video->attachPlayer(m_player);

    m_privacyBadge = new QLabel(tr("\u25CF 本地离线识别"), container);
    m_privacyBadge->setObjectName(QStringLiteral("privacyBadge"));
    m_privacyBadge->setAttribute(Qt::WA_TransparentForMouseEvents);

    QFrame* card = buildAsrCard(container);

    grid->addWidget(m_video, 0, 0);
    grid->addWidget(m_privacyBadge, 0, 0, Qt::AlignLeft | Qt::AlignTop);
    grid->addWidget(card, 0, 0, Qt::AlignRight | Qt::AlignTop);
    v->addWidget(container, 1);

    v->addWidget(buildControlDeck(stage));
    return stage;
}

QFrame* MainWindow::buildAsrCard(QWidget* parent) {
    QFrame* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("asrCard"));
    card->setFixedWidth(216);
    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(12, 10, 12, 12);
    v->setSpacing(6);

    auto* head = new QHBoxLayout();
    auto* title = new QLabel(tr("实时字幕"), card);
    title->setObjectName(QStringLiteral("asrHead"));
    m_asrDot = new QLabel(QStringLiteral("\u25CF"), card);
    m_asrDot->setStyleSheet(QStringLiteral("color:#757c88; font-size:10px;"));
    m_asrStatus = new QLabel(tr("未运行"), card);
    m_asrStatus->setStyleSheet(QStringLiteral("color:#757c88; font-size:11px; background:transparent;"));
    head->addWidget(title);
    head->addStretch();
    head->addWidget(m_asrDot);
    head->addWidget(m_asrStatus);
    v->addLayout(head);

    auto row = [this, card, v](const QString& k, const QString& val) {
        auto* h = new QHBoxLayout();
        auto* key = new QLabel(k, card);
        key->setProperty("class", "asrKey");
        auto* value = new QLabel(val, card);
        value->setProperty("class", "asrVal");
        value->setWordWrap(false);
        h->addWidget(key);
        h->addStretch();
        h->addWidget(value);
        v->addLayout(h);
        return value;
    };
    row(tr("引擎"), tr("Zipformer2-CTC + SenseVoice"));
    row(tr("语言"), tr("中文（auto）"));
    row(tr("模式"), tr("本地离线 · Balanced"));

    m_exportBtn = new QPushButton(tr("导出字幕（SRT）"), card);
    m_exportBtn->setCursor(Qt::PointingHandCursor);
    m_exportBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #5147aa, stop:1 #6254cb);"
        " border:none; border-radius:8px; color:white; padding:7px 10px; font-weight:600; }"
        "QPushButton:hover { background:#6254cb; }"));
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::onExportSrt);
    return card;
}

QWidget* MainWindow::buildControlDeck(QWidget* parent) {
    auto* deck = new QWidget(parent);
    deck->setObjectName(QStringLiteral("controlDeck"));
    auto* v = new QVBoxLayout(deck);
    v->setContentsMargins(14, 10, 14, 12);
    v->setSpacing(8);

    // 时间轴行
    auto* timeline = new QHBoxLayout();
    timeline->setSpacing(10);
    m_timeCur = new QLabel(tr("00:00"), deck);
    m_seek = new QSlider(Qt::Horizontal, deck);
    m_seek->setRange(0, 0);
    m_seek->setCursor(Qt::PointingHandCursor);
    m_timeDur = new QLabel(tr("00:00"), deck);
    timeline->addWidget(m_timeCur);
    timeline->addWidget(m_seek, 1);
    timeline->addWidget(m_timeDur);
    v->addLayout(timeline);

    // 控制行
    auto* row = new QHBoxLayout();
    row->setSpacing(8);

    m_captionBtn = new QPushButton(tr("字 幕"), deck);
    m_captionBtn->setProperty("class", "ctrlBtn");
    m_captionBtn->setCheckable(true);
    m_captionBtn->setChecked(true);
    m_captionBtn->setCursor(Qt::PointingHandCursor);
    m_captionBtn->setToolTip(tr("开关实时字幕叠加"));

    m_btnB10 = new QPushButton(QStringLiteral("\u23EA"), deck);
    m_playPauseBtn = new QPushButton(QStringLiteral("\u25B6"), deck);
    m_playPauseBtn->setObjectName(QStringLiteral("btnPlay"));
    m_btnF10 = new QPushButton(QStringLiteral("\u23E9"), deck);
    m_btnPrev = new QPushButton(QStringLiteral("\u23EE"), deck);
    m_btnNext = new QPushButton(QStringLiteral("\u23ED"), deck);
    m_btnFs = new QPushButton(QStringLiteral("\u26F6"), deck);

    for (auto* b : {m_btnB10, m_btnF10, m_btnPrev, m_btnNext, m_btnFs}) {
        b->setProperty("class", "ctrlBtn");
        b->setCursor(Qt::PointingHandCursor);
    }

    m_speedCombo = new QComboBox(deck);
    m_speedCombo->addItems({tr("0.5x"), tr("0.75x"), tr("1.0x"), tr("1.25x"), tr("1.5x"), tr("2.0x")});
    m_speedCombo->setCurrentIndex(2);
    m_player->setSpeed(1.0);

    m_volume = new QSlider(Qt::Horizontal, deck);
    m_volume->setRange(0, 100);
    m_volume->setValue(100);
    m_volume->setFixedWidth(96);
    m_volume->setCursor(Qt::PointingHandCursor);

    row->addWidget(m_captionBtn);
    row->addStretch();
    row->addWidget(m_btnB10);
    row->addWidget(m_btnPrev);
    row->addWidget(m_playPauseBtn);
    row->addWidget(m_btnNext);
    row->addWidget(m_btnF10);
    row->addStretch();
    row->addWidget(m_speedCombo);
    row->addWidget(new QLabel(tr("音量"), deck));
    row->addWidget(m_volume);
    row->addWidget(m_btnFs);
    v->addLayout(row);

    connect(m_captionBtn, &QPushButton::clicked, this, &MainWindow::onToggleCaption);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_btnB10, &QPushButton::clicked, this, [this] {
        m_player->seek(qBound(0.0, m_player->timePosition() - 10.0, m_duration), false);
    });
    connect(m_btnF10, &QPushButton::clicked, this, [this] {
        m_player->seek(qBound(0.0, m_player->timePosition() + 10.0, m_duration), false);
    });
    connect(m_btnPrev, &QPushButton::clicked, this, [this] { onPrevNext(-1); });
    connect(m_btnNext, &QPushButton::clicked, this, [this] { onPrevNext(1); });
    connect(m_btnFs, &QPushButton::clicked, this, &MainWindow::onToggleFullscreen);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSpeedChanged);
    connect(m_volume, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(m_seek, &QSlider::sliderMoved, this, &MainWindow::onSeekSlider);
    connect(m_seek, &QSlider::sliderReleased, this, &MainWindow::onSeekReleased);
    return deck;
}

void MainWindow::openFile(const QString& path) {
    if (!m_player || !m_player->handle()) return;
    if (m_player->loadFile(path)) {
        m_player->play();
        const QString name = QFileInfo(path).fileName();
        m_titleFile->setText(name);
        setWindowTitle(QStringLiteral("实时字幕播放器 — %1").arg(name));
        updatePlaylistHighlight(path);
        startCaptioningFor(path);
    } else {
        setAsrStatus(tr("加载失败"), QStringLiteral("#ff6b79"));
    }
}

void MainWindow::startCaptioningFor(const QString& path) {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString workerExe = appDir + QStringLiteral("/caption_worker.exe");
    // 打包布局（bundle/MSI）模型在 <appDir>/models；开发机布局在 <appDir>/.tools/models。
    QString modelsRoot = appDir + QStringLiteral("/models");
    if (!QFileInfo::exists(modelsRoot))
        modelsRoot = appDir + QStringLiteral("/.tools/models");

    if (m_worker->isRunning()) m_worker->shutdown();
    m_transcript->clear();
    m_finalCount = 0;
    m_stats->setText(tr("字幕行数：0"));

    if (!QFile::exists(workerExe)) {
        setAsrStatus(tr("缺少 caption_worker"), QStringLiteral("#ff6b79"));
        return;
    }
    m_worker->start(workerExe, QFileInfo(path).absoluteFilePath(), modelsRoot);
}

void MainWindow::appendTranscriptPartial(const QString& text) {
    if (text.isEmpty()) return;
    // 转写面板最后一行若是 partial，则原位更新；final 落地时固定。
    const int last = m_transcript->count() - 1;
    const bool lastIsPartial = (last >= 0 && m_transcript->item(last)->data(Qt::UserRole).toBool());
    if (lastIsPartial) {
        m_transcript->item(last)->setText(QStringLiteral("\u2026 ") + text);
        m_transcript->scrollToBottom();
        return;
    }
    auto* it = new QListWidgetItem(QStringLiteral("\u2026 ") + text, m_transcript);
    it->setData(Qt::UserRole, true);
    it->setForeground(QColor(0xa7, 0xad, 0xb8));
    m_transcript->scrollToBottom();
}

void MainWindow::appendTranscriptFinal(const QString& text, long long startMs) {
    // partial 行转正：若最后一行是 partial，先移除，final 文本更完整。
    const int last = m_transcript->count() - 1;
    if (last >= 0 && m_transcript->item(last)->data(Qt::UserRole).toBool())
        delete m_transcript->item(last);
    QString line;
    if (startMs >= 0) {
        line = QStringLiteral("[%1] %2").arg(formatTime(startMs / 1000.0), text);
    } else {
        line = text;   // 系统消息（错误/导出反馈）不带时间戳
    }
    auto* it = new QListWidgetItem(line, m_transcript);
    it->setData(Qt::UserRole, false);
    ++m_finalCount;
    m_stats->setText(tr("字幕行数：%1").arg(m_finalCount));
    m_transcript->scrollToBottom();
}

void MainWindow::setAsrStatus(const QString& text, const QString& color) {
    if (m_asrDot) m_asrDot->setStyleSheet(QStringLiteral("color:%1; font-size:10px; background:transparent;").arg(color));
    if (m_asrStatus) {
        m_asrStatus->setText(text);
        m_asrStatus->setStyleSheet(
            QStringLiteral("color:%1; font-size:11px; background:transparent;").arg(color));
    }
}

void MainWindow::updatePlaylistHighlight(const QString& path) {
    const int idx = m_mediaPaths.indexOf(path);
    if (idx < 0) {
        m_mediaPaths.append(path);
        m_playlist->addItem(QFileInfo(path).fileName());
        m_playlist->setCurrentRow(m_playlist->count() - 1);
    } else {
        m_playlist->setCurrentRow(idx);
    }
}

void MainWindow::onPlaylistActivated(int row) {
    if (row < 0 || row >= m_mediaPaths.size()) return;
    openFile(m_mediaPaths.at(row));
}

void MainWindow::onPrevNext(int delta) {
    if (m_mediaPaths.isEmpty()) return;
    int row = m_playlist->currentRow() + delta;
    row = qBound(0, row, m_mediaPaths.size() - 1);
    m_playlist->setCurrentRow(row);
    onPlaylistActivated(row);
}

void MainWindow::onOpen() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("打开媒体文件"),
        QString(), tr("媒体文件 (*.mp4 *.mkv *.avi *.mov *.webm *.mp3 *.mka *.wav);;所有文件 (*.*)"));
    if (!path.isEmpty()) {
        openFile(path);
        // 打开即入列（若已存在则复用高亮）
        if (!m_mediaPaths.contains(path)) {
            m_mediaPaths.append(path);
            m_playlist->addItem(QFileInfo(path).fileName());
            m_playlist->setCurrentRow(m_playlist->count() - 1);
        }
    }
}

void MainWindow::onPlayPause() {
    if (!m_player) return;
    m_player->togglePause();
}

void MainWindow::onStop() {
    if (!m_player) return;
    m_worker->stop();
    m_player->stop();
}

void MainWindow::onToggleFullscreen() {
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}

void MainWindow::onSpeedChanged(int index) {
    static const double speeds[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0};
    if (index >= 0 && index < 6) m_player->setSpeed(speeds[index]);
}

void MainWindow::onVolumeChanged(int value) {
    if (m_player) m_player->setVolume(value);
}

void MainWindow::onPositionChanged(double seconds) {
    if (m_seeking) return;
    if (m_duration > 0.0) {
        m_seek->setValue(static_cast<int>(seconds));
    }
    m_timeCur->setText(formatTime(seconds));
}

void MainWindow::onDurationChanged(double seconds) {
    m_duration = seconds;
    if (seconds > 0.0) {
        m_seek->setRange(0, static_cast<int>(seconds));
    }
    m_timeDur->setText(formatTime(seconds));
}

void MainWindow::onSeekSlider(int value) {
    m_seeking = true;
    m_timeCur->setText(formatTime(value));
}

void MainWindow::onSeekReleased() {
    if (!m_player) return;
    m_player->seek(m_seek->value(), false);
    m_seeking = false;
}

void MainWindow::onMediaLoaded() {
    m_playPauseBtn->setText(QStringLiteral("\u23F8"));
}

void MainWindow::onMediaEnded() {
    m_playPauseBtn->setText(QStringLiteral("\u25B6"));
    m_worker->stop();
    m_player->clearSubtitleOverlay();
}

void MainWindow::onExportSrt() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("导出字幕 SRT"), QString(), tr("SubRip (*.srt)"));
    if (path.isEmpty()) return;
    const QVector<rcp::CaptionSegment> fins = m_captionCtl->finals();
    QList<rcp::CaptionSegment> list;
    list.reserve(fins.size());
    for (const auto& s : fins) list.append(s);
    const auto res = rcp::captions::SrtExporter::writeSrt(path, list);
    if (res.isError())
        appendTranscriptFinal(tr("[导出失败] %1").arg(path), -1);
    else
        appendTranscriptFinal(tr("[已导出] %1").arg(path), -1);
}

void MainWindow::onToggleCaption() {
    m_captionOn = m_captionBtn->isChecked();
    if (!m_captionOn) {
        m_player->clearSubtitleOverlay();
    } else {
        m_captionCtl->setPlayheadMs(static_cast<long long>(m_player->timePosition() * 1000.0));
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    // 自绘标题栏：按住拖动移动窗口（FramelessWindowHint 时生效）。
    if (obj == m_titleBar && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            m_windowDrag = me->globalPosition().toPoint() - frameGeometry().topLeft();
        }
    } else if (obj == m_titleBar && event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->buttons() & Qt::LeftButton && !(windowState() & Qt::WindowMaximized)) {
            move(me->globalPosition().toPoint() - m_windowDrag);
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) openFile(urls.first().toLocalFile());
}

QString MainWindow::formatTime(double seconds) const {
    if (seconds < 0) seconds = 0;
    const int total = static_cast<int>(seconds);
    const int m = total / 60;
    const int s = total % 60;
    return QStringLiteral("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}
