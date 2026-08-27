// src/player/MainWindow.h
// 基础播放器主窗口：视频渲染区 + 传输控制栏（打开/播放暂停/停止/进度/
// 音量/倍速/静音）。本阶段仅打通"能打开视频并播放渲染"的播放内核；
// 字幕叠加与 worker 进程在后续里程碑接入。
#pragma once

#include <QMainWindow>

class MpvPlayer;
class MpvRenderWidget;
class QSlider;
class QPushButton;
class QLabel;
class QComboBox;

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

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void setupUi();
    QString formatTime(double seconds) const;

    MpvPlayer* m_player = nullptr;
    MpvRenderWidget* m_video = nullptr;

    QSlider* m_seek = nullptr;
    QPushButton* m_playPauseBtn = nullptr;
    QLabel* m_timeLabel = nullptr;
    QComboBox* m_speedCombo = nullptr;
    QSlider* m_volume = nullptr;
    QPushButton* m_muteBtn = nullptr;

    double m_duration = 0.0;
    bool m_seeking = false;
};
