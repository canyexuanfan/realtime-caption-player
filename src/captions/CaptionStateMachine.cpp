#include "captions/CaptionStateMachine.h"

namespace rcp::captions {

void CaptionStateMachine::reset(quint64 generation) {
    generation_ = generation;
    state_ = CaptionDisplayState::Listening;
    current_ = CaptionLine{};
}

void CaptionStateMachine::onPartial(const CaptionSegment& seg, quint64 generation) {
    if (!isCurrent(generation)) return; // discard stale generation
    state_ = CaptionDisplayState::Partial;
    current_.id = seg.id;
    current_.text = seg.text;
    current_.kind = CaptionKind::Partial;
    current_.startMs = seg.startMs;
    current_.endMs = seg.endMs;
}

void CaptionStateMachine::onFinal(const CaptionSegment& seg, quint64 generation) {
    if (!isCurrent(generation)) return;
    state_ = CaptionDisplayState::Final;
    current_.id = seg.id;
    current_.text = seg.text;
    current_.kind = CaptionKind::Final;
    current_.startMs = seg.startMs;
    current_.endMs = seg.endMs;
}

void CaptionStateMachine::onRevision(const CaptionSegment& seg, quint64 generation) {
    if (!isCurrent(generation)) return;
    // A revision updates an already-final line; keep the Final display state.
    if (state_ != CaptionDisplayState::Final) return;
    current_.id = seg.id;
    current_.text = seg.text;
    current_.kind = CaptionKind::Revision;
    current_.startMs = seg.startMs;
    current_.endMs = seg.endMs;
}

void CaptionStateMachine::onStop(quint64 generation) {
    if (!isCurrent(generation)) return;
    state_ = CaptionDisplayState::Idle;
    current_ = CaptionLine{};
}

} // namespace rcp::captions
