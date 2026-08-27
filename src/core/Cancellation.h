#pragma once

#include <memory>
#include <atomic>

namespace rcp {

/// Cooperative cancellation token. Cheap to copy; all copies share the same
/// underlying atomic flag owned by the originating CancellationSource.
class CancellationToken {
public:
    CancellationToken() : state_(std::make_shared<std::atomic<bool>>(false)) {}
    explicit CancellationToken(std::shared_ptr<std::atomic<bool>> s) : state_(std::move(s)) {}

    bool isCanceled() const noexcept {
        return state_ && state_->load(std::memory_order_acquire);
    }

    std::shared_ptr<std::atomic<bool>> state() const { return state_; }

private:
    std::shared_ptr<std::atomic<bool>> state_;
};

/// Source that triggers cancellation for every token it has handed out.
class CancellationSource {
public:
    CancellationSource() : state_(std::make_shared<std::atomic<bool>>(false)) {}

    CancellationToken token() const { return CancellationToken(state_); }

    void cancel() { state_->store(true, std::memory_order_release); }

    bool isCanceled() const noexcept {
        return state_->load(std::memory_order_acquire);
    }

private:
    std::shared_ptr<std::atomic<bool>> state_;
};

} // namespace rcp
