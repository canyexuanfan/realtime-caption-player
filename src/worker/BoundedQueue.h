#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>
#include <utility>
#include "core/Cancellation.h"

namespace rcp::worker {

/// How often a blocked push/pop re-checks an external CancellationToken.
/// Cancellation is cooperative: CancellationSource::cancel() only flips a flag
/// and does not notify condition variables, so we poll rather than wait forever.
inline constexpr std::chrono::milliseconds kCancelPollInterval{50};


/// Thread-safe bounded blocking queue with cooperative cancellation.
///
/// Used to move PCM chunks / decoded frames / ASR results between the
/// decoder, scheduler and recognizer threads with bounded memory (tech plan 8.6).
template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(size_t capacity) : capacity_(capacity ? capacity : 1) {}

    /// Push an item, blocking while the queue is full. Returns false if the
    /// queue was shut down or cancellation fired before space became available.
    bool push(T item, const CancellationToken& tok = {}) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (tok.isCanceled()) return false;
        notFull_.wait_for(lock, kCancelPollInterval, [&] {
            return shutdown_ || tok.isCanceled() || items_.size() < capacity_;
        });
        if (shutdown_ || tok.isCanceled()) return false;
        items_.push(std::move(item));
        notEmpty_.notify_one();
        return true;
    }

    /// Pop the next item, blocking while empty. Returns nullopt on shutdown
    /// or cancellation.
    std::optional<T> pop(const CancellationToken& tok = {}) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (tok.isCanceled()) return std::nullopt;
        notEmpty_.wait_for(lock, kCancelPollInterval, [&] {
            return shutdown_ || tok.isCanceled() || !items_.empty();
        });
        if (items_.empty()) return std::nullopt;
        T item = std::move(items_.front());
        items_.pop();
        notFull_.notify_one();
        return item;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

    bool empty() const { return size() == 0; }

    /// Wake all waiters so they observe shutdown/cancellation.
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        notFull_.notify_all();
        notEmpty_.notify_all();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::queue<T> items_;
    size_t capacity_;
    bool shutdown_ = false;
};

} // namespace rcp::worker
