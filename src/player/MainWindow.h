// src/player/MainWindow.h
// 主播放窗口 —— 按 05_Single_HTML_Frontend_Reference.html 复刻的暗色 UI：
// 自绘标题栏（品牌/文件名/打开/最小化/最大化/关闭）+ 左侧栏（播放列表 +
// 实时字幕转写面板）+ 中央视频区（mpv 渲染 + 隐私徽标 + 实时字幕状态浮卡）
// + 控制台（进度条 + 播放控制 + 倍速/音量/字幕开关/全屏）。
//
// 字幕链路：打开媒体 -> 启动后台 caption_worker (WorkerSupervisor) ->
// 接收逐句事件 -> CaptionController 按播放头对齐 -> mpv osd-overlay 叠加，
// 同时落入左侧"实时字幕"转写面板；并提供字幕开关与 SRT 导出。
#pragma once

#include <QMainWindow>
#include <QVector>

class MpvPlayer;
class MpvRenderWidget;
class QSlider;
class QPushButton;
class QLabel;
class QComboBox;
class QListWidget;
class QFrame;
namespace rcp::player { class WorkerSupervisor; }
namespace rcp::captions { class CaptionController; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

public slots:
    void openFile(const QString& path);   // 打开本地媒体文件

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onOpen();
    void onPlayPause();
    void onStop();
    void onPrevNext(int delta);
    void onSpeedChanged(int index);
    void onVolumeChanged(int value);
    void onPositionChanged(double seconds);
    void onDurationChanged(double seconds);
    void onSeekSlider(int value);
    void onSeekReleased();
    void onMediaLoaded();
    void onMediaEnded();
    void onExportSrt();
    void onToggleCaption();
    void onToggleFullscreen();
    void onPlaylistActivated(int row);

private:
    void setupUi();
    QWidget* buildTitleBar(QWidget* parent);
    QWidget* buildLeftRail(QWidget* parent);
    QWidget* buildStage(QWidget* parent);
    QFrame*  buildAsrCard(QWidget* parent);
    QWidget* buildControlDeck(QWidget* parent);
    void applyTheme();

    void appendTranscriptPartial(const QString& text);
    void appendTranscriptFinal(const QString& text, long long startMs);
    void setAsrStatus(const QString& text, const QString& color);
    void updatePlaylistHighlight(const QString& path);
    QString formatTime(double seconds) const;
    void startCaptioningFor(const QString& path);

    // ---- 播放与字幕链路 ----
    MpvPlayer* m_player = nullptr;
    MpvRenderWidget* m_video = nullptr;
    rcp::captions::CaptionController* m_captionCtl = nullptr;
    rcp::player::WorkerSupervisor* m_worker = nullptr;
    bool m_captionOn = true;
    QStringList m_mediaPaths;          // 播放列表（已打开文件）

    // ---- 标题栏 ----
    QWidget* m_titleBar = nullptr;
    QLabel* m_titleFile = nullptr;
    QPushButton* m_btnMin = nullptr;
    QPushButton* m_btnMax = nullptr;
    QPushButton* m_btnClose = nullptr;

    // ---- 左侧栏 ----
    QListWidget* m_playlist = nullptr;
    QListWidget* m_transcript = nullptr;
    QLabel* m_stats = nullptr;
    int m_finalCount = 0;

    // ---- 视频区浮层 ----
    QLabel* m_privacyBadge = nullptr;
    QLabel* m_asrDot = nullptr;
    QLabel* m_asrStatus = nullptr;

    // ---- 控制台 ----
    QSlider* m_seek = nullptr;
    QLabel* m_timeCur = nullptr;
    QLabel* m_timeDur = nullptr;
    QPushButton* m_playPauseBtn = nullptr;
    QComboBox* m_speedCombo = nullptr;
    QSlider* m_volume = nullptr;
    QPushButton* m_captionBtn = nullptr;
    QPushButton* m_btnF10 = nullptr;
    QPushButton* m_btnB10 = nullptr;
    QPushButton* m_btnPrev = nullptr;
    QPushButton* m_btnNext = nullptr;
    QPushButton* m_btnFs = nullptr;
    QPushButton* m_exportBtn = nullptr;

    double m_duration = 0.0;
    bool m_seeking = false;
    QPoint m_windowDrag;
};
