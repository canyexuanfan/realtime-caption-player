#include "ipc/FrameCodec.h"
#include "ipc/Protocol.h"

#include <QtEndian>

namespace rcp::ipc {

void FrameDecoder::reset() {
    buf_.clear();
}

bool FrameDecoder::feed(const QByteArray& data, QList<QByteArray>& frames, QString* errorOut) {
    buf_.append(data);
    // Extract as many complete frames as available.
    for (;;) {
        if (buf_.size() < 4) return true; // need length prefix
        const quint32 len = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(buf_.constData()));
        if (len == 0) {
            if (errorOut) *errorOut = QStringLiteral("IPC-FRAME-EMPTY");
            return false;
        }
        if (len > kMaxFrameBytes) {
            if (errorOut) *errorOut = QStringLiteral("IPC-FRAME-TOO-LARGE");
            return false;
        }
        if (static_cast<quint32>(buf_.size()) < 4u + len) return true; // wait for full payload
        const QByteArray payload = buf_.mid(4, static_cast<int>(len));
        buf_.remove(0, 4 + static_cast<int>(len));
        frames.append(payload);
    }
}

QByteArray FrameDecoder::encodeFrame(const QByteArray& jsonPayload) {
    QByteArray out;
    const quint32 len = static_cast<quint32>(jsonPayload.size());
    uchar prefix[4];
    qToLittleEndian<quint32>(len, prefix);
    out.append(reinterpret_cast<const char*>(prefix), 4);
    out.append(jsonPayload);
    return out;
}

} // namespace rcp::ipc
