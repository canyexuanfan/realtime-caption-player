#include "settings/KeymapService.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>

namespace rcp::settings {

Result<void> KeymapService::load(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return Result<void>::fail(AppError(ErrorDomain::App,
            QStringLiteral("KEYMAP-READ-FAILED"),
            QStringLiteral("无法读取快捷键配置"),
            QStringLiteral("QFile::open failed")));
    const QByteArray raw = f.readAll();
    f.close();

    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);
    if (doc.isNull())
        return Result<void>::fail(AppError(ErrorDomain::App,
            QStringLiteral("KEYMAP-PARSE-FAILED"),
            QStringLiteral("快捷键配置格式损坏"),
            QStringLiteral("JSON parse error at %1").arg(perr.offset)));
    if (!doc.isObject())
        return Result<void>::fail(AppError(ErrorDomain::App,
            QStringLiteral("KEYMAP-PARSE-FAILED"),
            QStringLiteral("快捷键配置格式损坏"), QStringLiteral("root not object")));
    return loadFromObject(doc.object());
}

Result<void> KeymapService::loadFromObject(const QJsonObject& root) {
    const QJsonValue bindings = root.value(QStringLiteral("bindings"));
    if (!bindings.isObject())
        return Result<void>::fail(AppError(ErrorDomain::App,
            QStringLiteral("KEYMAP-INVALID"),
            QStringLiteral("快捷键配置缺少 bindings"),
            QStringLiteral("bindings not object")));
    const QJsonObject b = bindings.toObject();
    QMap<QString, QString> parsed;
    for (auto it = b.begin(); it != b.end(); ++it) {
        if (!it.value().isString())
            return Result<void>::fail(AppError(ErrorDomain::App,
                QStringLiteral("KEYMAP-INVALID"),
                QStringLiteral("快捷键绑定值必须为字符串"),
                QStringLiteral("action ") + it.key()));
        parsed.insert(it.key(), it.value().toString());
    }
    bindings_ = std::move(parsed);
    return Result<void>::ok();
}

void KeymapService::loadDefaults() {
    bindings_.clear();
    bindings_.insert(QStringLiteral("playback.toggle_pause"), QStringLiteral("Space"));
    bindings_.insert(QStringLiteral("playback.seek_backward_5s"), QStringLiteral("Left"));
    bindings_.insert(QStringLiteral("playback.seek_forward_5s"), QStringLiteral("Right"));
    bindings_.insert(QStringLiteral("playback.speed_down"), QStringLiteral("["));
    bindings_.insert(QStringLiteral("playback.speed_up"), QStringLiteral("]"));
    bindings_.insert(QStringLiteral("caption.toggle"), QStringLiteral("Ctrl+Shift+C"));
    bindings_.insert(QStringLiteral("caption.export_srt"), QStringLiteral("Ctrl+Shift+S"));
    bindings_.insert(QStringLiteral("window.fullscreen"), QStringLiteral("F11"));
}

bool KeymapService::hasAction(const QString& action) const {
    return bindings_.contains(action);
}

QString KeymapService::keyForAction(const QString& action) const {
    return bindings_.value(action, QString());
}

QKeySequence KeymapService::sequenceForAction(const QString& action) const {
    const QString k = keyForAction(action);
    if (k.isEmpty()) return QKeySequence();
    return QKeySequence::fromString(k, QKeySequence::NativeText);
}

QStringList KeymapService::actions() const {
    return QStringList(bindings_.keys());
}

QList<QKeySequence> KeymapService::allSequences() const {
    QList<QKeySequence> out;
    for (const QString& k : bindings_)
        out.append(QKeySequence::fromString(k, QKeySequence::NativeText));
    return out;
}

QStringList KeymapService::conflictBindings() const {
    QMap<QString, int> counts;
    for (const QString& k : bindings_)
        counts[k] += 1;
    QStringList conflicts;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (it.value() > 1) conflicts.append(it.key());
    }
    return conflicts;
}

} // namespace rcp::settings
