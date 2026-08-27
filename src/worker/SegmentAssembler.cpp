#include "worker/SegmentAssembler.h"

namespace rcp::worker {

void SegmentAssembler::addPartial(const CaptionSegment& partial) {
    if (partial.generation != generation_ && generation_ != 0) return; // stale generation
    pending_ = partial;
    pending_.kind = CaptionKind::Partial;
    hasPending_ = true;
}

AssembleResult SegmentAssembler::addFinal(const CaptionSegment& final) {
    AssembleResult r;
    if (final.generation != generation_ && generation_ != 0) return r; // stale

    CaptionSegment seg = final;
    seg.kind = CaptionKind::Final;
    committed_.insert(seg.id, seg);
    r.action = AssembleResult::Action::Committed;
    r.segment = seg;
    r.id = seg.id;

    // If this final closes the pending utterance, clear it.
    if (hasPending_ && pending_.id == seg.id) hasPending_ = false;
    return r;
}

AssembleResult SegmentAssembler::addRevision(const CaptionSegment& revision) {
    AssembleResult r;
    if (revision.generation != generation_ && generation_ != 0) return r; // stale detection
    if (!committed_.contains(revision.id)) return r; // nothing to revise

    CaptionSegment seg = revision;
    seg.kind = CaptionKind::Revision;
    committed_[revision.id] = seg;
    r.action = AssembleResult::Action::Revised;
    r.segment = seg;
    r.id = seg.id;
    return r;
}

AssembleResult SegmentAssembler::tick(qint64 nowMs, qint64 finalTimeoutMs) {
    AssembleResult r;
    if (!hasPending_) return r;
    if (nowMs - pending_.endMs < finalTimeoutMs) return r;

    CaptionSegment seg = pending_;
    seg.kind = CaptionKind::Final; // force-commit best-so-far
    committed_.insert(seg.id, seg);
    r.action = AssembleResult::Action::Committed;
    r.segment = seg;
    r.id = seg.id;
    hasPending_ = false;
    return r;
}

void SegmentAssembler::reset(quint64 generation) {
    generation_ = generation;
    pending_ = CaptionSegment{};
    hasPending_ = false;
    committed_.clear();
}

} // namespace rcp::worker
