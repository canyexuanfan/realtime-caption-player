#include "ipc/Protocol.h"

#include <QHash>

namespace rcp::ipc {

QString commandTypeName(CommandType t) {
    return wireTypeForCommand(t);
}

CommandType commandTypeFromName(const QString& name) {
    static const QHash<QString, CommandType> map = {
        {QStringLiteral("command.hello"),             CommandType::Hello},
        {QStringLiteral("command.load_model_bundle"), CommandType::LoadModelBundle},
        {QStringLiteral("command.open_media"),        CommandType::OpenMedia},
        {QStringLiteral("command.close_media"),       CommandType::CloseMedia},
        {QStringLiteral("command.set_playhead"),      CommandType::SetPlayhead},
        {QStringLiteral("command.seek"),              CommandType::Seek},
        {QStringLiteral("command.set_paused"),        CommandType::SetPaused},
        {QStringLiteral("command.set_speed"),         CommandType::SetSpeed},
        {QStringLiteral("command.set_audio_track"),   CommandType::SetAudioTrack},
        {QStringLiteral("command.set_language"),      CommandType::SetLanguage},
        {QStringLiteral("command.set_profile"),       CommandType::SetProfile},
        {QStringLiteral("command.start_captioning"),  CommandType::StartCaptioning},
        {QStringLiteral("command.stop_captioning"),   CommandType::StopCaptioning},
        {QStringLiteral("command.shutdown"),          CommandType::Shutdown},
    };
    auto it = map.find(name);
    if (it != map.end()) return it.value();
    return CommandType::Hello; // first enum value = invalid sentinel
}

QString eventTypeName(EventType e) {
    return wireTypeForEvent(e);
}

EventType eventTypeFromName(const QString& name) {
    static const QHash<QString, EventType> map = {
        {QStringLiteral("event.ready"),               EventType::Ready},
        {QStringLiteral("event.model_progress"),      EventType::ModelProgress},
        {QStringLiteral("event.media_opened"),        EventType::MediaOpened},
        {QStringLiteral("event.caption_partial"),     EventType::CaptionPartial},
        {QStringLiteral("event.caption_final"),       EventType::CaptionFinal},
        {QStringLiteral("event.caption_revision"),    EventType::CaptionRevision},
        {QStringLiteral("event.coverage_progress"),   EventType::CoverageProgress},
        {QStringLiteral("event.overload"),            EventType::Overload},
        {QStringLiteral("event.metrics"),             EventType::Metrics},
        {QStringLiteral("event.heartbeat"),           EventType::Heartbeat},
        {QStringLiteral("event.error"),               EventType::Error},
    };
    auto it = map.find(name);
    if (it != map.end()) return it.value();
    return EventType::Ready; // first enum value = invalid sentinel
}

QString wireTypeForCommand(CommandType t) {
    switch (t) {
    case CommandType::Hello:            return QStringLiteral("command.hello");
    case CommandType::LoadModelBundle:  return QStringLiteral("command.load_model_bundle");
    case CommandType::OpenMedia:        return QStringLiteral("command.open_media");
    case CommandType::CloseMedia:       return QStringLiteral("command.close_media");
    case CommandType::SetPlayhead:      return QStringLiteral("command.set_playhead");
    case CommandType::Seek:             return QStringLiteral("command.seek");
    case CommandType::SetPaused:        return QStringLiteral("command.set_paused");
    case CommandType::SetSpeed:         return QStringLiteral("command.set_speed");
    case CommandType::SetAudioTrack:    return QStringLiteral("command.set_audio_track");
    case CommandType::SetLanguage:      return QStringLiteral("command.set_language");
    case CommandType::SetProfile:       return QStringLiteral("command.set_profile");
    case CommandType::StartCaptioning:  return QStringLiteral("command.start_captioning");
    case CommandType::StopCaptioning:   return QStringLiteral("command.stop_captioning");
    case CommandType::Shutdown:         return QStringLiteral("command.shutdown");
    }
    return QStringLiteral("command.unknown");
}

QString wireTypeForEvent(EventType e) {
    switch (e) {
    case EventType::Ready:             return QStringLiteral("event.ready");
    case EventType::ModelProgress:     return QStringLiteral("event.model_progress");
    case EventType::MediaOpened:       return QStringLiteral("event.media_opened");
    case EventType::CaptionPartial:    return QStringLiteral("event.caption_partial");
    case EventType::CaptionFinal:      return QStringLiteral("event.caption_final");
    case EventType::CaptionRevision:   return QStringLiteral("event.caption_revision");
    case EventType::CoverageProgress:  return QStringLiteral("event.coverage_progress");
    case EventType::Overload:          return QStringLiteral("event.overload");
    case EventType::Metrics:           return QStringLiteral("event.metrics");
    case EventType::Heartbeat:         return QStringLiteral("event.heartbeat");
    case EventType::Error:             return QStringLiteral("event.error");
    }
    return QStringLiteral("event.unknown");
}

} // namespace rcp::ipc
