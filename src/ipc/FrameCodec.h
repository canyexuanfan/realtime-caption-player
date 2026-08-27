#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace rcp::ipc {

/// Streaming decoder for length-prefixed JSON frames over QLocalSocket.
/// Frame = uint32 LE length + `length` bytes of UTF-8 JSON.
/// Handles split/coalesced writes, oversize frames, and empty frames.
class FrameDecoder {
public:
    FrameDecoder() = default;

    /// Reset all buffered state (e.g. on reconnect).
    void reset();

    /// Feed raw bytes. Extracts any complete frame *payloads* (without the
    /// 4-byte length prefix) into `frames`. Returns false if a fatal framing
    /// error occurred (oversize or invalid length); `errorOut` is set and the
    /// caller should drop the connection (record IPC-FRAME-TOO-LARGE etc.).
    bool feed(const QByteArray& data, QList<QByteArray>& frames, QString* errorOut = nullptr);

    /// Total buffered bytes (for diagnostics / backpressure).
    int bufferedBytes() const { return buf_.size(); }

    /// Encode a JSON payload with its length prefix (no validation of JSON).
    static QByteArray encodeFrame(const QByteArray& jsonPayload);

private:
    QByteArray buf_;
};

} // namespace rcp::ipc
