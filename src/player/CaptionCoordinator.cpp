// src/player/CaptionCoordinator.cpp
#include "CaptionCoordinator.h"
#include "WorkerSupervisor.h"

#include <algorithm>

namespace rcp::player {

CaptionCoordinator::CaptionCoordinator(QObject* parent) : QObject(parent) {
    m_supervisor = new WorkerSupervisor(this);
    connect(m_supervisor, &WorkerSupervisor::sessionStarted, this, [this](quint64) {
        m_finals.clear();
        m_partial = rcp::CaptionSegment{};
        m_havePartial = false;
        m_coveredUntilMs = 0;
        m_lastFinalText.clear();
        setState(State::Running);
        emit finalsChanged();
        emit statsChanged(0, 0);
        recompute();
    });
    connect(m_supervisor, &WorkerSupervisor::captionSegment, this, &CaptionCoordinator::handleSegment);
    connect(m_supervisor, &WorkerSupervisor::ready, this, [this] {
        if (m_state != State::Error) setState(State::Running);
    });
    connect(m_supervisor, &WorkerSupervisor::workerError, this, [this](const QString& msg) {
        m_lastError = msg;
        setState(State::Error, msg);
    });
    connect(m_supervisor, &WorkerSupervisor::workerFinished, this, [this] {
        if (m_state != State::Disabled && m_state != State::Error) setState(State::Suspended);
    });
    connect(m_supervisor, &WorkerSupervisor::captioningStopped, this, [this] {
        m_havePartial = false;
        recompute();
    });
    // B5 过载自动降级：RTF 持续超阈值 → 本会话内一次性降为 Lite（跳过终稿精修）。
    connect(m_supervisor, &WorkerSupervisor::overloadDetected, this, [this](double rtf) {
        if (m_autoDowngraded) return;
        m_autoDowngraded = true;
        setProfile(QStringLiteral("lite"));
        emit transcriptFinal(-1, QStringLiteral("[过载] 实时率 %1，已自动切换到 Lite 档位").arg(rtf, 0, 'f', 2));
    });
}

CaptionCoordinator::~CaptionCoordinator() = default;

void CaptionCoordinator::setWorkerExecutable(const QString& exe, const QString& modelsRoot) {
    m_workerExe = exe;
    m_modelsRoot = modelsRoot;
}

void CaptionCoordinator::setState(State s, const QString& msg) {
    if (m_state == s && msg == m_lastError) return;
    m_state = s;
    if (s == State::Error) m_lastError = msg;
    emit stateChanged(s, msg.isEmpty() ? m_lastError : msg);
}

void CaptionCoordinator::openMedia(const QString& path) {
    bumpGeneration();
    m_finals.clear();
    m_partial = rcp::CaptionSegment{};
    m_havePartial = false;
    m_coveredUntilMs = 0;
    m_lastFinalText.clear();
    m_autoDowngraded = false;
    emit finalsChanged();
    emit statsChanged(0, 0);
    setState(State::Starting);
    m_supervisor->start(m_workerExe, path, m_modelsRoot, m_audioTrackFfIndex, m_generation);
    recompute();
}

void CaptionCoordinator::stop() {
    m_supervisor->stop();
    setState(State::Suspended);
}

void CaptionCoordinator::shutdown() {
    m_supervisor->shutdown();
    setState(State::Disabled);
}

void CaptionCoordinator::setPlayheadMs(long long rawMs) {
    m_playheadMs = rawMs;
    if (m_state == State::Seeking) setState(State::Running);
    recompute();
}

void CaptionCoordinator::seekedToMs(long long rawMs) {
    if (m_state == State::Disabled || m_state == State::Error) return;
    bumpGeneration();
    m_playheadMs = rawMs;
    m_havePartial = false;
    setState(State::Seeking);
    m_supervisor->seek(rawMs, m_generation);
    recompute();
}

void CaptionCoordinator::setPaused(bool paused) {
    m_supervisor->setPaused(paused);
}

void CaptionCoordinator::setSpeed(double speed) {
    m_supervisor->setSpeed(speed);
}

void CaptionCoordinator::setAudioTrack(int ffIndex) {
    if (ffIndex == m_audioTrackFfIndex && m_state == State::Disabled) return;
    m_audioTrackFfIndex = ffIndex;
    if (m_state == State::Running || m_state == State::Starting || m_state == State::Seeking) {
        bumpGeneration();
        setState(State::Seeking);
        m_supervisor->setAudioTrack(ffIndex, m_generation);
    }
}

void CaptionCoordinator::setLanguage(const QString& lang) {
    bumpGeneration();
    m_supervisor->setLanguage(lang, m_generation);
}

void CaptionCoordinator::setProfile(const QString& profile) {
    bumpGeneration();
    m_supervisor->setProfile(profile, m_generation);
}

void CaptionCoordinator::setDelayMs(long long delayMs) {
    m_delayMs = delayMs;
    recompute();
}

void CaptionCoordinator::setEnabled(bool on) {
    m_enabled = on;
    recompute();
}

void CaptionCoordinator::refresh() {
    recompute();
}

void CaptionCoordinator::handleSegment(const rcp::CaptionSegment& seg, bool isPartial) {
    if (seg.generation != 0 && seg.generation != m_generation) return; // 显示侧代隔离兜底
    if (isPartial) {
        m_partial = seg;
        m_havePartial = true;
        emit transcriptPartial(seg.text);
    } else {
        // 有序插入（startMs 升序），显示/导出用同一份时间线（ADR-0007）。
        auto it = std::lower_bound(m_finals.begin(), m_finals.end(), seg.startMs,
                                   [](const rcp::CaptionSegment& s, long long v) { return s.startMs < v; });
        if (it != m_finals.end() && it->id == seg.id) *it = seg;   // 修订覆盖
        else m_finals.insert(it, seg);
        if (seg.endMs > m_coveredUntilMs) m_coveredUntilMs = seg.endMs;
        if (!seg.text.isEmpty()) m_lastFinalText = seg.text;
        emit transcriptFinal(seg.startMs, seg.text);
        emit finalsChanged();
        emit statsChanged(m_finals.size(), m_coveredUntilMs);
    }
    recompute();
}

void CaptionCoordinator::recompute() {
    if (!m_enabled) {
        emit displayChanged(QString(), QString());
        return;
    }
    const long long head = m_playheadMs + m_delayMs;

    // final：显示"最后一条已开始"的句子（startMs <= head+0.5s，显示到下一句开始）；
    // 无已到达句时兜底常驻最后一句（防空窗）。二分查找（时间线升序）。
    QString finalText;
    auto it = std::upper_bound(m_finals.cbegin(), m_finals.cend(), head + 500,
                               [](long long v, const rcp::CaptionSegment& s) { return v < s.startMs; });
    if (it != m_finals.cbegin()) finalText = (it - 1)->text;
    if (finalText.isEmpty()) finalText = m_lastFinalText;

    // partial：仅当播放头落在其区间内（+0.5s 容差）才显示，杜绝"未来字幕"。
    QString partialText;
    if (m_havePartial && head >= m_partial.startMs - 500 && head <= m_partial.endMs + 500)
        partialText = m_partial.text;

    emit displayChanged(partialText, finalText);
}

} // namespace rcp::player
