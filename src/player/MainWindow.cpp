// src/player/MainWindow.cpp
#include "MainWindow.h"
#include "MpvPlayer.h"
#include "MpvRenderWidget.h"

#include <QApplication>
#include <QFileDialog>
#include <QToolBar>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QStatusBar>
#include <QUrl>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QStyle>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_player = new MpvPlayer(this);

    if (!m_player->create()) {
        // 沙箱/异常环境下句柄创建失败：仍可构建 UI，但播放会无效。
        statusBar()->showMessage(tr("警告：libmpv 句柄创建失败"));
    } else {
        m_player->setOption(QStringLiteral("vo"), QStringLiteral("libmpv"));
        m_player->setOption(QStringLiteral("hwdec"), QStringLiteral("no"));
        m_player->initialize();
    }

    setupUi();

    // 信号接线
    connect(m_player, &MpvPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &MpvPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player, &MpvPlayer::mediaLoaded, this, &MainWindow::onMediaLoaded);
    connect(m_player, &MpvPlayer::mediaEnded, this, &MainWindow::onMediaEnded);

    setAcceptDrops(true);
    resize(960, 600);
    setWindowTitle(QApplication::applicationName());
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* vbox = new QVBoxLayout(central);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    m_video = new MpvRenderWidget(central);
    m_video->setMinimumSize(320, 240);
    vbox->addWidget(m_video, 1);

    if (m_player && m_player->handle()) {
        m_video->attachPlayer(m_player);
    }

    // 传输控制栏
    auto* bar = new QWidget(central);
    auto* hbox = new QHBoxLayout(bar);
    hbox->setContentsMargins(8, 4, 8, 4);
    hbox->setSpacing(6);

    auto* openBtn = new QPushButton(style()->standardIcon(QStyle::SP_DirOpenIcon), tr("打开"), bar);
    m_playPauseBtn = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), tr("播放"), bar);
    auto* stopBtn = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop), tr("停止"), bar);

    m_seek = new QSlider(Qt::Horizontal, bar);
    m_seek->setRange(0, 0);
    m_timeLabel = new QLabel(tr("00:00 / 00:00"), bar);

    m_speedCombo = new QComboBox(bar);
    m_speedCombo->addItems({tr("0.5x"), tr("0.75x"), tr("1.0x"), tr("1.25x"), tr("1.5x"), tr("2.0x")});
    m_speedCombo->setCurrentIndex(2);
    m_player->setSpeed(1.0);

    m_volume = new QSlider(Qt::Horizontal, bar);
    m_volume->setRange(0, 100);
    m_volume->setValue(100);
    m_muteBtn = new QPushButton(tr("静音"), bar);

    hbox->addWidget(openBtn);
    hbox->addWidget(m_playPauseBtn);
    hbox->addWidget(stopBtn);
    hbox->addWidget(m_seek, 1);
    hbox->addWidget(m_timeLabel);
    hbox->addWidget(new QLabel(tr("速度"), bar));
    hbox->addWidget(m_speedCombo);
    hbox->addWidget(new QLabel(tr("音量"), bar));
    hbox->addWidget(m_volume);
    hbox->addWidget(m_muteBtn);

    vbox->addWidget(bar);

    setCentralWidget(central);

    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpen);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSpeedChanged);
    connect(m_volume, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(m_muteBtn, &QPushButton::clicked, this, [this] {
        static bool muted = false;
        muted = !muted;
        m_player->setMuted(muted);
        m_muteBtn->setText(muted ? tr("取消静音") : tr("静音"));
    });
    connect(m_seek, &QSlider::sliderMoved, this, &MainWindow::onSeekSlider);
    connect(m_seek, &QSlider::sliderReleased, this, &MainWindow::onSeekReleased);
}

void MainWindow::openFile(const QString& path) {
    if (!m_player || !m_player->handle()) return;
    if (m_player->loadFile(path)) {
        m_player->play();
        statusBar()->showMessage(tr("正在播放：%1").arg(path));
    } else {
        statusBar()->showMessage(tr("加载失败：%1").arg(path));
    }
}

void MainWindow::onOpen() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("打开媒体文件"),
        QString(), tr("媒体文件 (*.mp4 *.mkv *.avi *.mov *.webm *.mp3 *.mka *.wav);;所有文件 (*.*)"));
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::onPlayPause() {
    if (!m_player) return;
    m_player->togglePause();
}

void MainWindow::onStop() {
    if (!m_player) return;
    m_player->stop();
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
    m_timeLabel->setText(tr("%1 / %2").arg(formatTime(seconds), formatTime(m_duration)));
}

void MainWindow::onDurationChanged(double seconds) {
    m_duration = seconds;
    if (seconds > 0.0) {
        m_seek->setRange(0, static_cast<int>(seconds));
    }
    m_timeLabel->setText(tr("%1 / %2").arg(formatTime(m_player->timePosition()), formatTime(seconds)));
}

void MainWindow::onSeekSlider(int value) {
    m_seeking = true;
    m_timeLabel->setText(tr("%1 / %2").arg(formatTime(value), formatTime(m_duration)));
}

void MainWindow::onSeekReleased() {
    if (!m_player) return;
    m_player->seek(m_seek->value(), false);
    m_seeking = false;
}

void MainWindow::onMediaLoaded() {
    m_playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    m_playPauseBtn->setText(tr("暂停"));
}

void MainWindow::onMediaEnded() {
    m_playPauseBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playPauseBtn->setText(tr("播放"));
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
