// src/player/MainWindow.h
// 主播放窗口 —— 逐像素复刻 05_Single_HTML_Frontend_Reference.html（除实例数据外）：
// 标题栏（logo+品牌+文件名+打开文件/打开文件夹/设置+最小化/最大化/关闭）；
// 左侧栏 238px（播放列表+历史记录 tabs、实时字幕转写面板、搜索框、行数/延迟统计）；
// 舞台（mpv 视频区 + 文件名/隐私徽标/字幕叠加(灰字partial+36px白字final描边)/
// 波形/可拖动实时字幕状态浮卡）；控制台（时间轴、三组圆形控制钮、倍速弹窗、
// 音量、字幕轨、AB循环、截图、全屏）；右侧 344px 设置面板（7 页导航）。
#pragma once

#include <QMainWindow>
#include <QPointer>
#include <QSettings>
#include <QVector>

#include "captions/CaptionTypes.h"

class MpvPlayer;
class MpvRenderWidget;
class QSlider;
class QPushButton;
class QLabel;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QFrame;
class QLineEdit;
class QStackedWidget;
class QTimer;
class QDragLeaveEvent;
namespace rcpui { class Switch; class CaptionLabel; class Waveform; }
namespace rcp::player { class WorkerSupervisor; }
namespace rcp::captions { class CaptionController; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

public slots:
    void openFile(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onOpen();
    void onOpenFolder();
    void onToggleSettings();
    void onPlayPause();
    void onPrevNext(int delta);
    void onSpeed(double v);
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
    void onPlaylistActivated(QListWidgetItem* item);
    void onScreenshot();
    void onAbLoop();
    void onCycleSub();
    void onTranscriptSearch(const QString& text);

private:
    void applyTheme();
    void buildTitleBar(QWidget* parent);
    QWidget* buildLeftRail(QWidget* parent);
    QWidget* buildStage(QWidget* parent);
    QWidget* buildVideoArea(QWidget* parent);
    QWidget* buildControlDeck(QWidget* parent);
    QWidget* buildSettingsPanel(QWidget* parent);
    QWidget* wrapPaneScroll(QWidget* inner);
    QWidget* paneGeneral();
    QWidget* panePlayback();
    QWidget* paneSubtitle();
    QWidget* paneRealtime();
    QWidget* paneShortcuts();
    QWidget* paneAppearance();
    QWidget* paneAdvanced();

    QWidget* sectionTitle(const QString& text, QWidget* parent);
    QWidget* settingRow(const QString& text, QWidget* editor, QWidget* parent,
                        const QString& help = QString());
    QWidget* navButton(const QString& icon, const QString& text);

    void appendTranscriptPartial(const QString& text);
    void appendTranscriptFinal(long long startMs, const QString& text);
    QString formatTime(double seconds) const;
    void setAsrStatus(const QString& text, const QString& color);
    void addMediaPaths(const QStringList& paths);
    void refreshMediaRows();
    void loadHistory();
    void saveHistory(const QString& path);
    void updateOverlay();
    void applyCaptionStyle();
    void repositionOverlays();
    void updateDeckForState();

    // ---- 播放与字幕链路 ----
    MpvPlayer* m_player = nullptr;
    MpvRenderWidget* m_video = nullptr;
    QWidget* m_videoHost = nullptr;   // 视频容器（离屏快照模式下为黑色占位）
    rcp::captions::CaptionController* m_captionCtl = nullptr;
    rcp::player::WorkerSupervisor* m_worker = nullptr;
    QSettings m_settings;
    QStringList m_mediaPaths;
    QVector<rcp::CaptionSegment> m_finals;   // 本窗口维护的 final 时间线
    QString m_partialText;
    long long m_delayMs = 0;               // 字幕同步延迟
    long long m_lastFinalEndMs = 0;
    bool m_captionOn = true;

    // ---- 标题栏 ----
    QWidget* m_titleBar = nullptr;
    QLabel* m_titleFile = nullptr;
    QPushButton* m_btnSettingsTitle = nullptr;

    // ---- 左侧栏 ----
    QListWidget* m_mediaList = nullptr;
    QListWidget* m_historyList = nullptr;
    QStackedWidget* m_mediaStack = nullptr;
    QLabel* m_libTitle = nullptr;
    QListWidget* m_transcript = nullptr;
    QPointer<QLabel> m_lastPartialLabel;
    QLabel* m_statLines = nullptr;
    QLabel* m_statLatency = nullptr;
    QPushButton* m_tabPlaylist = nullptr;
    QPushButton* m_tabHistory = nullptr;
    int m_finalCount = 0;

    // ---- 视频区浮层 ----
    QWidget* m_captionOverlay = nullptr;
    rcpui::CaptionLabel* m_partialLabel = nullptr;
    rcpui::CaptionLabel* m_finalLabel = nullptr;
    rcpui::Waveform* m_waveform = nullptr;
    QLabel* m_asrDot = nullptr;
    QLabel* m_asrStatus = nullptr;
    QLabel* m_asrEngine = nullptr;
    QLabel* m_asrConf = nullptr;
    QLabel* m_asrLatency = nullptr;
    QLabel* m_asrRecognized = nullptr;
    QWidget* m_privacyBadge = nullptr;
    QLabel* m_videoFileTitle = nullptr;
    QLabel* m_vignette = nullptr;
    QLabel* m_dropHint = nullptr;
    QString m_currentPath;

    // ---- 控制台 ----
    QSlider* m_seek = nullptr;
    QLabel* m_timeCur = nullptr;
    QLabel* m_timeDur = nullptr;
    QPushButton* m_playBtn = nullptr;
    QPushButton* m_btnCaption = nullptr;
    QPushButton* m_btnCam = nullptr;
    QPushButton* m_btnAb = nullptr;
    QPushButton* m_btnTrack = nullptr;
    QPushButton* m_btnB10 = nullptr;
    QPushButton* m_btnF10 = nullptr;
    QPushButton* m_btnPrev = nullptr;
    QPushButton* m_btnNext = nullptr;
    QPushButton* m_btnSpeed = nullptr;
    QPushButton* m_btnSettings = nullptr;
    QSlider* m_volume = nullptr;
    QPushButton* m_btnFs = nullptr;
    QLabel* m_speedText = nullptr;
    int m_abClicks = 0;

    // ---- 设置面板 ----
    QWidget* m_settingsPanel = nullptr;
    QStackedWidget* m_panes = nullptr;
    QList<QPair<QPushButton*, int>> m_navBtns;

    double m_duration = 0.0;
    bool m_seeking = false;
    QPoint m_windowDrag;
};
