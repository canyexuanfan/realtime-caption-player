#include "ipc/JsonMessageCodec.h"
#include "ipc/FrameCodec.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace rcp::ipc {

namespace {
AppError ipcError(const QString& detail) {
    return AppError(ErrorDomain::Ipc, QStringLiteral("IPC-PROTOCOL-MISMATCH"),
                   QStringLiteral("字幕组件版本不匹配"), detail);
}
} // namespace

QByteArray JsonMessageCodec::serialize(const Envelope& env) {
    QJsonObject root;
    root[QStringLiteral("v")] = env.version;
    root[QStringLiteral("type")] = env.type;
    root[QStringLiteral("id")] = env.id;
    root[QStringLiteral("generation")] = static_cast<qint64>(env.generation);
    root[QStringLiteral("payload")] = env.payload;
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Compact);
    return FrameDecoder::encodeFrame(json);
}

Result<Envelope> JsonMessageCodec::parse(const QByteArray& framePayload) {
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(framePayload, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        return Result<Envelope>::fail(AppError(ErrorDomain::Ipc,
            QStringLiteral("IPC-PROTOCOL-MISMATCH"),
            QStringLiteral("字幕组件版本不匹配"),
            QStringLiteral("bad JSON: %1").arg(perr.errorString())));
    }
    const QJsonObject root = doc.object();
    Envelope env;
    // v
    if (root.contains(QStringLiteral("v"))) {
        const QJsonValue v = root.value(QStringLiteral("v"));
        if (!v.isDouble()) return Result<Envelope>::fail(ipcError(QStringLiteral("missing/invalid 'v'")));
        env.version = static_cast<quint8>(v.toInt());
    }
    // type (required)
    {
        const QJsonValue t = root.value(QStringLiteral("type"));
        if (!t.isString() || t.toString().isEmpty())
            return Result<Envelope>::fail(ipcError(QStringLiteral("missing/invalid 'type'")));
        env.type = t.toString();
    }
    // id (required)
    {
        const QJsonValue id = root.value(QStringLiteral("id"));
        if (!id.isString() || id.toString().isEmpty())
            return Result<Envelope>::fail(ipcError(QStringLiteral("missing/invalid 'id'")));
        env.id = id.toString();
    }
    // generation
    if (root.contains(QStringLiteral("generation"))) {
        const QJsonValue g = root.value(QStringLiteral("generation"));
        if (!g.isDouble()) return Result<Envelope>::fail(ipcError(QStringLiteral("invalid 'generation'")));
        env.generation = static_cast<quint64>(g.toInteger());
    }
    // payload (optional, default {})
    if (root.contains(QStringLiteral("payload"))) {
        const QJsonValue p = root.value(QStringLiteral("payload"));
        if (!p.isObject()) return Result<Envelope>::fail(ipcError(QStringLiteral("invalid 'payload'")));
        env.payload = p.toObject();
    }
    if (!env.isValid()) return Result<Envelope>::fail(ipcError(QStringLiteral("envelope invalid")));
    return Result<Envelope>::ok(env);
}

bool JsonMessageCodec::isObjectJson(const QByteArray& framePayload) {
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(framePayload, &perr);
    return perr.error == QJsonParseError::NoError && doc.isObject();
}

} // namespace rcp::ipc
