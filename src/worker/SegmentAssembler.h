#pragma once

#include "captions/CaptionTypes.h"
#include <QMap>

namespace rcp::worker {

/// Result of feeding a result into the assembler.
struct AssembleResult {
    enum class Action { None, Committed, Revised };
    Action action = Action::None;
    CaptionSegment segment; ///< the committed/revised segment when action != None
    QString id;             ///< id of the affected segment
};

/// Assembles streaming partials into committed final segments, and applies
/// late revisions. Honors generation isolation (tech plan 8.x / 28).
class SegmentAssembler {
public:
    SegmentAssembler() = default;

    /// Feed an interim partial for the current utterance.
    void addPartial(const CaptionSegment& partial);
    /// Feed a final utterance; commits the current pending segment.
    AssembleResult addFinal(const CaptionSegment& final);
    /// Feed a late revision of an already-committed segment.
    AssembleResult addRevision(const CaptionSegment& revision);

    /// Periodic tick. If the current utterance has partials but no final within
    /// `finalTimeoutMs`, force-commit the best-so-far text (tech plan 8.x timeout).
    AssembleResult tick(qint64 nowMs, qint64 finalTimeoutMs = 2500);

    /// Drop all pending/committed state for a new generation.
    void reset(quint64 generation);

    bool hasPending() const { return hasPending_; }
    quint64 generation() const { return generation_; }

private:
    quint64 generation_ = 0;
    CaptionSegment pending_;
    bool hasPending_ = false;
    QMap<CaptionId, CaptionSegment> committed_;
};

} // namespace rcp::worker
