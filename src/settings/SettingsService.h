#pragma once

#include "settings/Settings.h"
#include "core/Result.h"
#include <QString>

namespace rcp::settings {

/// Atomic, redaction-safe settings persistence (tech plan 10.2 / 10.4).
class SettingsService {
public:
    /// Load settings from `path`. Missing file -> defaults (non-fatal).
    static Result<Settings> load(const QString& path);
    /// Load, but treat a missing file as an error (used at startup validation).
    static Result<Settings> loadStrict(const QString& path);
    /// Atomically write settings (temp file + rename). Never partial on crash.
    static Result<void> save(const QString& path, const Settings& s);
    /// Convenience overload: persist the default settings to `path`.
    static Result<void> save(const QString& path);
};

} // namespace rcp::settings
