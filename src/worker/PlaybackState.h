// src/worker/PlaybackState.h
// 播放头共享状态：IPC 主线程写、解码/ASR 线程读（tech plan 7.7/8.6）。
#pragma once

#include <mutex>
#include <cstdint>

namespace rcp::worker {

class PlaybackState {
public:
    void update(std::int64_t playheadMs, double speed, bool paused) {
        std::lock_guard<std::mutex> lock(m_);
        playheadMs_ = playheadMs;
        speed_ = speed > 0 ? speed : 1.0;
        paused_ = paused;
    }
    void setGeneration(std::uint64_t g) {
        std::lock_guard<std::mutex> lock(m_);
        generation_ = g;
    }

    struct Snapshot {
        std::int64_t playheadMs = 0;
        double speed = 1.0;
        bool paused = false;
        std::uint64_t generation = 0;
    };
    Snapshot snapshot() const {
        std::lock_guard<std::mutex> lock(m_);
        return {playheadMs_, speed_, paused_, generation_};
    }

private:
    mutable std::mutex m_;
    std::int64_t playheadMs_ = 0;
    double speed_ = 1.0;
    bool paused_ = false;
    std::uint64_t generation_ = 0;
};

} // namespace rcp::worker
