#pragma once

#include "captions/CaptionTypes.h"
#include <cstdint>

namespace rcp::captions {

/// Display lifecycle of the live caption overlay (tech plan 6.x / 28).
enum class CaptionDisplayState {
    Idle,      ///< no caption active (not captioning, or stopped)
    Listening, ///< captioning active, waiting for first result
    Partial,   ///< showing an interim hypothesis
    Final      ///< showing a committed / revised final
};

inline QString captionDisplayStateName(CaptionDisplayState s) {
    switch (s) {
    case CaptionDisplayState::Idle:      return QStringLiteral("idle");
    case CaptionDisplayState::Listening: return QStringLiteral("listening");
    case CaptionDisplayState::Partial:   return QStringLiteral("partial");
    case CaptionDisplayState::Final:     return QStringLiteral("final");
    }
    return QStringLiteral("idle");
}

/// Drives the caption overlay state from worker events, isolating stale
/// generations (seek / track change / worker restart must discard old events).
class CaptionStateMachine {
public:
    CaptionStateMachine() = default;

    void reset(quint64 generation);                 // start of a captioning session
    void onPartial(const CaptionSegment& seg, quint64 generation);
    void onFinal(const CaptionSegment& seg, quint64 generation);
    void onRevision(const CaptionSegment& seg, quint64 generation);
    void onStop(quint64 generation);                // captioning stopped / closed

    CaptionDisplayState state() const { return state_; }
    CaptionLine currentLine() const { return current_; }
    bool hasActiveCaption() const { return state_ != CaptionDisplayState::Idle; }
    quint64 generation() const { return generation_; }

private:
    bool isCurrent(quint64 g) const { return g == generation_; }

    quint64 generation_ = 0;
    CaptionDisplayState state_ = CaptionDisplayState::Idle;
    CaptionLine current_;
};

} // namespace rcp::captions
