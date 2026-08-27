#pragma once

#include "core/Error.h"
#include <variant>
#include <utility>
#include <type_traits>

namespace rcp {

/// Result<T>: explicit success/failure value, never an exception for expected errors.
template <typename T>
class Result {
public:
    static Result ok(T value) { return Result(std::in_place_type<T>, std::move(value)); }
    static Result fail(AppError err) { return Result(std::in_place_type<AppError>, std::move(err)); }

    bool isOk() const noexcept { return std::holds_alternative<T>(data_); }
    bool isError() const noexcept { return std::holds_alternative<AppError>(data_); }

    const T& value() const {
        if (!isOk()) throw std::logic_error("Result::value() called on error result");
        return std::get<T>(data_);
    }
    T take() {
        if (!isOk()) throw std::logic_error("Result::take() called on error result");
        return std::move(std::get<T>(data_));
    }
    const AppError& error() const {
        if (!isError()) throw std::logic_error("Result::error() called on ok result");
        return std::get<AppError>(data_);
    }

private:
    explicit Result(std::in_place_type_t<T>, T v) : data_(std::move(v)) {}
    explicit Result(std::in_place_type_t<AppError>, AppError e) : data_(std::move(e)) {}
    std::variant<T, AppError> data_;
};

/// Result<void>: success or failure without a payload.
template <>
class Result<void> {
public:
    static Result ok() { return Result(); }
    static Result fail(AppError err) { return Result(std::move(err)); }

    bool isOk() const noexcept { return !err_.has_value(); }
    bool isError() const noexcept { return err_.has_value(); }
    const AppError& error() const {
        if (!err_) throw std::logic_error("Result<void>::error() on ok result");
        return *err_;
    }

private:
    Result() = default;
    explicit Result(AppError e) : err_(std::move(e)) {}
    std::optional<AppError> err_;
};

} // namespace rcp
