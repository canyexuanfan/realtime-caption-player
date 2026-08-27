#pragma once

#include "core/Result.h"
#include <QString>
#include <QStringList>
#include <QKeySequence>
#include <QMap>

namespace rcp::settings {

/// Loads and resolves keyboard bindings (tech plan 6.10).
/// Actions are dotted names like "playback.toggle_pause".
class KeymapService {
public:
    KeymapService() = default;

    /// Load from a keymap JSON file.
    Result<void> load(const QString& path);
    /// Load directly from a parsed object (tests / embedded defaults).
    Result<void> loadFromObject(const QJsonObject& root);
    /// Start from the built-in default bindings.
    void loadDefaults();

    bool hasAction(const QString& action) const;
    /// Raw key string for an action (e.g. "Ctrl+Shift+C"), empty if unbound.
    QString keyForAction(const QString& action) const;
    /// Resolved Qt key sequence (empty if unbound or unparseable).
    QKeySequence sequenceForAction(const QString& action) const;

    /// All known action names.
    QStringList actions() const;
    /// All resolved key sequences (for global shortcut registration).
    QList<QKeySequence> allSequences() const;

    /// Bindings claimed by more than one action (conflict = broken UX).
    QStringList conflictBindings() const;

private:
    QMap<QString, QString> bindings_; // action -> key string
};

} // namespace rcp::settings
