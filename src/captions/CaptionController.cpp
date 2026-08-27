// src/captions/CaptionController.cpp
#include "captions/CaptionController.h"
#include "captions/AssEscaper.h"

namespace rcp::captions {

void CaptionController::reset(quint64 generation) {
    m_generation = generation;
    m_fsm.reset(generation);
    m_finals.clear();
    m_partial = CaptionSegment{};
    m_havePartial = false;
    m_playheadMs = 0;
    recompute();
}

void CaptionController::onPartial(const CaptionSegment& seg, quint64 generation) {
    if (generation != m_generation) return; // 丢弃过期会话
    m_partial = seg;
    m_havePartial = true;
    recompute();
}

void CaptionController::onFinal(const CaptionSegment& seg, quint64 generation) {
    if (generation != m_generation) return;
    m_finals.append(seg);
    emit captionListChanged();
    recompute();
}

void CaptionController::onRevision(const CaptionSegment& seg, quint64 generation) {
    if (generation != m_generation) return;
    // 优先按 id 更新既有 final；否则按时间覆盖最后一条。
    bool updated = false;
    for (auto& f : m_finals) {
        if (f.id == seg.id) {
            f.text = seg.text;
            f.startMs = seg.startMs;
            f.endMs = seg.endMs;
            updated = true;
            break;
        }
    }
    if (!updated && !m_finals.isEmpty()) {
        m_finals.last().text = seg.text;
    }
    emit captionListChanged();
    recompute();
}

void CaptionController::onStop(quint64 generation) {
    if (generation != m_generation) return;
    m_fsm.onStop(generation);
    m_havePartial = false;
    recompute();
}

void CaptionController::setPlayheadMs(long long ms) {
    m_playheadMs = ms;
    recompute();
}

CaptionSegment CaptionController::activeAt(long long ms) const {
    // 优先命中 final：播放头落在 [start,end] 内。
    for (const auto& f : m_finals) {
        if (ms >= f.startMs && ms <= f.endMs) return f;
    }
    // 无 final 命中，且存在带时间戳的 partial（播放头落在其区间）时显示 partial。
    // 注意：worker 全速模式下 partial 时间戳通常超前播放头，故需 startMs>0 才显示，
    // 避免把"未来"的临时识别结果当成当前字幕。
    if (m_havePartial && m_partial.startMs > 0 &&
        ms >= m_partial.startMs && ms <= m_partial.endMs) {
        return m_partial;
    }
    CaptionSegment none;
    none.startMs = 1;
    none.endMs = 0;   // endMs < startMs => isValid() == false（无命中哨兵）
    return none;
}

void CaptionController::recompute() {
    const CaptionSegment active = activeAt(m_playheadMs);
    if (!active.isValid()) {
        // 无命中（静音/间隙/未识别）：切回 Idle 并清除叠加层（finals 仍保留供导出）。
        m_fsm.onStop(m_generation);
        emit overlayChanged(QString());
        return;
    }
    if (active.kind == CaptionKind::Partial)
        m_fsm.onPartial(active, m_generation);
    else
        m_fsm.onFinal(active, m_generation);
    emit overlayChanged(currentAssEvents());
}

QString CaptionController::currentAssEvents() const {
    const CaptionLine& line = m_fsm.currentLine();
    if (line.text.isEmpty()) return QString();
    const QString escaped = AssEscaper::escape(line.text);
    // mpv osd-overlay (ass-events) 单条 Dialogue；\an2 = 底部居中。
    return QStringLiteral("Dialogue: 0,0:00:00.00,0:00:00.00,Default,,0,0,0,,{\\an2}%1")
        .arg(escaped);
}

} // namespace rcp::captions
