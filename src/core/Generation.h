#pragma once

#include <cstdint>
#include <QtCore/qglobal.h>

namespace rcp {

/// Monotonic epoch counter used to invalidate stale work across seeks,
/// track changes, model changes and worker restarts (tech plan 7.4 / 8.x).
///
/// Every async operation captures the generation it was created under and
/// discards its result if a newer generation is active by the time it lands.
struct Generation {
    quint64 value = 0;

    constexpr Generation() = default;
    explicit constexpr Generation(quint64 v) : value(v) {}

    /// Produce the next generation (e.g. after a seek or track switch).
    Generation next() const { return Generation(value + 1); }

    /// True when this generation is older than `current` and therefore stale.
    bool isStale(quint64 current) const { return value < current; }

    bool operator==(const Generation& o) const { return value == o.value; }
    bool operator!=(const Generation& o) const { return value != o.value; }
    bool operator<(const Generation& o) const { return value < o.value; }
};

} // namespace rcp
