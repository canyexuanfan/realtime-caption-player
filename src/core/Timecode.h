#pragma once

#include <QString>
#include <chrono>

namespace rcp {

using Milliseconds = std::chrono::milliseconds;

/// Format a duration as SRT timestamp "HH:MM:SS,mmm".
/// Hours are NOT zero-padded beyond two digits (per tech plan: hours may exceed 99).
QString formatSrtTime(Milliseconds ms);

/// Format a duration as a human clock "H:MM:SS" (or "MM:SS" under one hour).
QString formatClock(Milliseconds ms);

/// Parse an SRT-style timestamp "HH:MM:SS,mmm" or "HH:MM:SS.mmm".
/// Returns negative on parse failure.
Milliseconds parseSrtTime(const QString& text);

/// Clamp a time into [0, maxMs]; used for export range limiting.
Milliseconds clampTime(Milliseconds ms, Milliseconds maxMs);

} // namespace rcp
