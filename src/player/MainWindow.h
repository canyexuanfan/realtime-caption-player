// src/player/MainWindow.h
// 主播放窗口：视频渲染区 + 传输控制栏 + 实时字幕链路。
// 字幕链路：打开媒体 -> 启动后台 caption_worker (WorkerSupervisor) ->
// 接收逐句事件 -> CaptionController 按播放头对齐 -> mpv osd-overlay 叠加；
// 并提供字幕开关与 SRT 导出。
#pragma once

#include <QMainWindow>

class MpvPlayer;
class MpvRenderWidget;
class QSlider;
class QPushButton;
class QLabel;
class QComboBox;
namespace rcp::player { class WorkerSupervisor; }
namespace rcp::captions { class CaptionController; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

public slots:
    void openFile(const QString& path);   // 打开本地媒体文件

private slots:
    void onOpen();
    void onPlayPause();
    void onStop();
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

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void setupUi();
    QString formatTime(double seconds) const;
    void startCaptioningFor(const QString& path);

    MpvPlayer* m_player = nullptr;
    MpvRenderWidget* m_video = nullptr;
    rcp::captions::CaptionController* m_captionCtl = nullptr;
    rcp::player::WorkerSupervisor* m_worker = nullptr;
    bool m_captionOn = true;

    QSlider* m_seek = nullptr;
    QPushButton* m_playPauseBtn = nullptr;
    QLabel* m_timeLabel = nullptr;
    QComboBox* m_speedCombo = nullptr;
    QSlider* m_volume = nullptr;
    QPushButton* m_muteBtn = nullptr;
    QPushButton* m_captionBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;

    double m_duration = 0.0;
    bool m_seeking = false;
};
