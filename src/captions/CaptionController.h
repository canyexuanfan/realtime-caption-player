// src/captions/CaptionController.h
// 字幕显示控制器：把 worker 回报的逐句事件（partial/final）按"播放头时间"
// 对齐为当前应当显示的叠加行，并维护 committed final 时间线供 SRT 导出。
//
// 同步模型（MVP）：worker 以全速预识别整段媒体并打绝对时间戳，本控制器在
// 播放头移动时挑选 [startMs,endMs] 覆盖当前时刻的 segment 作为 active 行。
// 真正的"跟随播放头抽音"实时同步留待真机调优（沙箱无法验证播放，标注 PARTIAL）。
#pragma once

#include "captions/CaptionStateMachine.h"
#include "captions/CaptionTypes.h"
#include <QObject>
#include <QVector>

namespace rcp::captions {

class CaptionController : public QObject {
    Q_OBJECT
public:
    explicit CaptionController(QObject* parent = nullptr) : QObject(parent) {}

    // 开始一段新的字幕会话（新的 generation 会丢弃上一会话的事件）。
    void reset(quint64 generation);

    void onPartial(const CaptionSegment& seg, quint64 generation);
    void onFinal(const CaptionSegment& seg, quint64 generation);
    void onRevision(const CaptionSegment& seg, quint64 generation);
    void onStop(quint64 generation);

    // 播放头推进（毫秒）。触发重新对齐并回报叠加层变化。
    void setPlayheadMs(long long ms);

    QVector<CaptionSegment> finals() const { return m_finals; }

    // 当前叠加层对应的 mpv osd-overlay（ass-events 格式）字符串；
    // 空串表示应清除叠加层。
    QString currentAssEvents() const;

signals:
    void overlayChanged(const QString& assEvents); // 空串 => 清除
    void captionListChanged();                      // finals 变化（供 SRT 导出 UI）

private:
    CaptionSegment activeAt(long long ms) const;
    void recompute();

    CaptionStateMachine m_fsm;
    QVector<CaptionSegment> m_finals;
    CaptionSegment m_partial;
    bool m_havePartial = false;
    long long m_playheadMs = 0;
    quint64 m_generation = 0;
};

} // namespace rcp::captions
