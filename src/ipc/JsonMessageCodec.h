#pragma once

#include "core/Result.h"
#include "ipc/Protocol.h"
#include <QByteArray>

namespace rcp::ipc {

/// Serialize / parse the JSON IPC envelope (tech plan 7.2).
class JsonMessageCodec {
public:
    /// Build a wire frame: [uint32 LE length][UTF-8 JSON] with v/type/id/generation/payload.
    static QByteArray serialize(const Envelope& env);

    /// Parse a single complete frame payload (length prefix already stripped).
    /// Returns AppError (domain Ipc) on malformed JSON or schema violation.
    static Result<Envelope> parse(const QByteArray& framePayload);

    /// Convenience: parse then re-serialize round-trips identically for valid input.
    static bool isObjectJson(const QByteArray& framePayload);
};

} // namespace rcp::ipc
