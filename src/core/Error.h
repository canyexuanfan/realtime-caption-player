#pragma once

#include <QString>
#include <QVariantMap>

namespace rcp {

enum class ErrorDomain {
    App,
    Playback,
    Caption,
    Model,
    Ipc,
    Storage,
    Export,
    Install,
    Rclone
};

/// Unified structured error. Technical details go to logs only; userMessage is shown.
struct AppError {
    ErrorDomain domain = ErrorDomain::App;
    QString code;            // e.g. "PLY-OPEN-FAILED", "IPC-WORKER-DIED"
    QString userMessage;     // actionable, human readable
    QString technicalMessage;// details for logs (already redacted)
    bool retryable = false;
    QVariantMap context;     // already redacted (no full paths / caption text)

    AppError() = default;
    AppError(ErrorDomain d, QString c, QString user, QString tech = {}, bool retry = false)
        : domain(d), code(std::move(c)), userMessage(std::move(user)),
          technicalMessage(std::move(tech)), retryable(retry) {}

    QString domainName() const;
    QString toString() const;
};

/// Shortcuts for the most common codes referenced in the technical plan.
namespace errc {
AppError playbackOpenFailed(const QString& detail);
AppError trackMissing();
AppError modelMissing();
AppError modelHashMismatch();
AppError ipcWorkerDied();
AppError ipcProtocolMismatch();
AppError asrAudioStreamNotFound();
AppError asrMediaNotSeekable();
AppError asrOverload();
AppError dbMigrationFailed();
AppError exportWriteDenied();
AppError rcloneCacheOff();
} // namespace errc

} // namespace rcp
