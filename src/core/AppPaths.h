#pragma once

#include "core/Result.h"
#include <QString>

namespace rcp {

/// Platform-resolved application directories (tech plan 10.3).
/// All paths are created lazily via ensureDir / ensureAll.
class AppPaths {
public:
    /// Per-user roaming data (database, settings overrides).
    static QString dataDir();
    /// Configuration directory (settings.json, keymap.json).
    static QString configDir();
    /// Local cache (downloaded models cache, transcript cache, logs).
    static QString cacheDir();
    /// Log output directory.
    static QString logDir();
    /// Downloaded / extracted ASR model bundles.
    static QString modelsDir();
    /// Cached final transcripts keyed by media identity (tech plan 11.2).
    static QString transcriptCacheDir();
    /// Scratch space for atomic writes and temp extractions.
    static QString tempDir();

    /// Create the directory (and parents) if missing. Fails with a redacted error.
    static Result<void> ensureDir(const QString& dir);
    /// Ensure every standard directory exists.
    static Result<void> ensureAll();

private:
    static QString subDir(const QString& base, const QString& name);
};

} // namespace rcp
