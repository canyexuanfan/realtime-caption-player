#pragma once

#include <QString>
#include <QJsonObject>
#include <cstdint>

namespace rcp::ipc {

constexpr quint32 kMaxFrameBytes = 1 * 1024 * 1024; // 1 MiB hard cap (tech plan 7.2)
constexpr quint8 kProtocolVersion = 1;

/// Commands: main process -> worker
enum class CommandType {
    Hello,
    LoadModelBundle,
    OpenMedia,
    CloseMedia,
    SetPlayhead,
    Seek,
    SetPaused,
    SetSpeed,
    SetAudioTrack,
    SetLanguage,
    SetProfile,
    StartCaptioning,
    StopCaptioning,
    Shutdown
};

/// Events: worker -> main process
enum class EventType {
    Ready,
    ModelProgress,
    MediaOpened,
    CaptionPartial,
    CaptionFinal,
    CaptionRevision,
    CoverageProgress,
    Overload,
    Metrics,
    Heartbeat,
    Error
};

QString commandTypeName(CommandType t);
CommandType commandTypeFromName(const QString& name); // empty/"unknown" -> invalid
QString eventTypeName(EventType t);
EventType eventTypeFromName(const QString& name);

/// Canonical wire type strings (tech plan 7.3)
QString wireTypeForCommand(CommandType t); // "command.open_media"
QString wireTypeForEvent(EventType e);     // "event.caption_final"

/// Strongly-typed IPC envelope (tech plan 7.2).
struct Envelope {
    quint8 version = kProtocolVersion;
    QString type;        // canonical wire string, e.g. "command.open_media"
    QString id;          // correlation id
    quint64 generation = 0;
    QJsonObject payload;

    bool isValid() const {
        return version == kProtocolVersion
            && !type.isEmpty()
            && !id.isEmpty();
    }
};

} // namespace rcp::ipc
