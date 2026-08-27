#include "core/Error.h"

namespace rcp {

QString AppError::domainName() const {
    switch (domain) {
        case ErrorDomain::App: return QStringLiteral("App");
        case ErrorDomain::Playback: return QStringLiteral("Playback");
        case ErrorDomain::Caption: return QStringLiteral("Caption");
        case ErrorDomain::Model: return QStringLiteral("Model");
        case ErrorDomain::Ipc: return QStringLiteral("Ipc");
        case ErrorDomain::Storage: return QStringLiteral("Storage");
        case ErrorDomain::Export: return QStringLiteral("Export");
        case ErrorDomain::Install: return QStringLiteral("Install");
        case ErrorDomain::Rclone: return QStringLiteral("Rclone");
    }
    return QStringLiteral("Unknown");
}

QString AppError::toString() const {
    return QStringLiteral("[%1] %2: %3").arg(domainName(), code, userMessage);
}

namespace errc {

AppError playbackOpenFailed(const QString& detail) {
    return AppError(ErrorDomain::Playback, QStringLiteral("PLY-OPEN-FAILED"),
                    QStringLiteral("无法打开该媒体文件"), detail, /*retryable=*/true);
}
AppError trackMissing() {
    return AppError(ErrorDomain::Playback, QStringLiteral("PLY-TRACK-MISSING"),
                    QStringLiteral("当前音轨已不可用，请重新选择"), QString(), /*retryable=*/true);
}
AppError modelMissing() {
    return AppError(ErrorDomain::Model, QStringLiteral("MOD-MISSING"),
                    QStringLiteral("实时字幕模型未安装完整"), QString(), /*retryable=*/false);
}
AppError modelHashMismatch() {
    return AppError(ErrorDomain::Model, QStringLiteral("MOD-HASH-MISMATCH"),
                    QStringLiteral("模型文件损坏"), QString(), /*retryable=*/false);
}
AppError ipcWorkerDied() {
    return AppError(ErrorDomain::Ipc, QStringLiteral("IPC-WORKER-DIED"),
                    QStringLiteral("字幕引擎已停止，正在恢复"), QString(), /*retryable=*/true);
}
AppError ipcProtocolMismatch() {
    return AppError(ErrorDomain::Ipc, QStringLiteral("IPC-PROTOCOL-MISMATCH"),
                    QStringLiteral("字幕组件版本不匹配"), QString(), /*retryable=*/false);
}
AppError asrAudioStreamNotFound() {
    return AppError(ErrorDomain::Caption, QStringLiteral("ASR-AUDIO-STREAM-NOT-FOUND"),
                    QStringLiteral("无法读取当前音轨"), QString(), /*retryable=*/true);
}
AppError asrMediaNotSeekable() {
    return AppError(ErrorDomain::Caption, QStringLiteral("ASR-MEDIA-NOT-SEEKABLE"),
                    QStringLiteral("此媒体无法从新位置继续识别"), QString(), /*retryable=*/false);
}
AppError asrOverload() {
    return AppError(ErrorDomain::Caption, QStringLiteral("ASR-OVERLOAD"),
                    QStringLiteral("设备处理速度不足，已切换轻量模式"), QString(), /*retryable=*/false);
}
AppError dbMigrationFailed() {
    return AppError(ErrorDomain::Storage, QStringLiteral("DB-MIGRATION-FAILED"),
                    QStringLiteral("历史数据暂时不可用"), QString(), /*retryable=*/false);
}
AppError exportWriteDenied() {
    return AppError(ErrorDomain::Export, QStringLiteral("EXP-WRITE-DENIED"),
                    QStringLiteral("无法写入所选位置"), QString(), /*retryable=*/true);
}
AppError rcloneCacheOff() {
    return AppError(ErrorDomain::Rclone, QStringLiteral("RCLONE-CACHE-OFF"),
                    QStringLiteral("当前挂载未启用完整读缓存，拖动可能卡顿"), QString(), /*retryable=*/false);
}

} // namespace errc

} // namespace rcp
