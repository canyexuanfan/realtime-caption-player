// src/player/CaptionCoordinator.h
// 字幕协调器（B3，审查阶段 B3 / tech plan 9.1）：主进程字幕唯一入口。
// 合并 WorkerSupervisor 的会话控制语义与 CaptionController 的时间线职责：
//   - 完整状态机 Disabled/Starting/Running/Seeking/Suspended/Error；
//   - 唯一时间线（finals 按 startMs 升序，二分查找）；
//   - generation 隔离：openMedia/seekedToMs/换轨/换语言/换档位时换代，
//     worker 侧旧代事件已被两端丢弃，显示侧再按代丢弃兜底；
//   - 显示选择：final 显示"最后一条已开始"的句子（show-until-next），
//     partial 仅当播放头落在其区间内（+0.5s 容差），杜绝"未来字幕"；
//     无已到达 final 时兜底常驻最后一句（防止空窗）。
// MainWindow 只与本类对话（ADR-0007：字幕时间线只保留一份）。
#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "captions/CaptionTypes.h"

namespace rcp::player {

class WorkerSupervisor;

class CaptionCoordinator : public QObject {
    Q_OBJECT
public:
    enum class State { Disabled, Starting, Running, Seeking, Suspended, Error };
    Q_ENUM(State)

    explicit CaptionCoordinator(QObject* parent = nullptr);
    ~CaptionCoordinator() override;

    void setWorkerExecutable(const QString& exe, const QString& modelsRoot);
    void openMedia(const QString& path);   // 打开媒体：新会话（generation+1）
    void stop();                            // 停止识别（保留 worker 进程）
    void shutdown();                        // 终止 worker 进程

    void setPlayheadMs(long long rawMs);    // mpv 播放头（原始媒体时间，不含延迟）
    void seekedToMs(long long rawMs);       // 用户 seek：generation+1 并清 partial
    void setPaused(bool paused);
    void setSpeed(double speed);
    void setAudioTrack(int ffIndex);        // ffmpeg 流索引（-1 = 首条音轨）
    void setLanguage(const QString& lang);  // "auto"/"zh"/"en"
    void setProfile(const QString& profile);// "balanced"/"lite"
    void setDelayMs(long long delayMs);
    void setEnabled(bool on);               // 字幕总开关：仅控显示层，不停止识别
    void refresh();                         // 样式/开关变化后强制重发 displayChanged

    QVector<rcp::CaptionSegment> finals() const { return m_finals; }
    State state() const { return m_state; }
    QString lastError() const { return m_lastError; }
    long long coveredUntilMs() const { return m_coveredUntilMs; }

signals:
    void displayChanged(const QString& partialText, const QString& finalText);
    void finalsChanged();
    void stateChanged(rcp::player::CaptionCoordinator::State state, const QString& message);
    void transcriptPartial(const QString& text);
    void transcriptFinal(long long startMs, const QString& text);
    void statsChanged(int finalCount, long long coveredUntilMs);

private:
    void bumpGeneration() { ++m_generation; }
    void setState(State s, const QString& msg = {});
    void recompute();
    void handleSegment(const rcp::CaptionSegment& seg, bool isPartial);

    WorkerSupervisor* m_supervisor = nullptr;
    QVector<rcp::CaptionSegment> m_finals;  // 按 startMs 升序（唯一时间线）
    rcp::CaptionSegment m_partial;
    bool m_havePartial = false;
    long long m_playheadMs = 0;   // 原始媒体播放头
    long long m_delayMs = 0;      // 字幕同步延迟（只作用于显示选择）
    long long m_coveredUntilMs = 0;
    quint64 m_generation = 0;
    State m_state = State::Disabled;
    QString m_lastError;
    QString m_workerExe;
    QString m_modelsRoot;
    bool m_enabled = true;
    int m_audioTrackFfIndex = -1; // 选定音轨的 ffmpeg 流索引（-1 = 首条）
    QString m_lastFinalText;      // 空窗兜底：最后到达的一句
};

} // namespace rcp::player
