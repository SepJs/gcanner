#pragma once

#include <game_req_analyzer/core/pch.hpp>

namespace game_req {

template<typename E>
class Unexpected {
public:
    Unexpected() = default;
    Unexpected(const E& error) : error_(error) {}
    Unexpected(E&& error) : error_(std::move(error)) {}
    
    template<typename... Args>
    Unexpected(std::in_place_t, Args&&... args) : error_(std::forward<Args>(args)...) {}
    
    [[nodiscard]] const E& error() const& noexcept { return error_; }
    [[nodiscard]] E& error() & noexcept { return error_; }
    [[nodiscard]] E&& error() && noexcept { return std::move(error_); }
    [[nodiscard]] const E&& error() const&& noexcept { return std::move(error_); }

private:
    std::remove_cvref_t<E> error_;
};

template<typename E>
[[nodiscard]] Unexpected<std::remove_cvref_t<E>> make_unexpected(E&& error) {
    return Unexpected<std::remove_cvref_t<E>>(std::forward<E>(error));
}

template<typename T, typename E>
class Expected {
public:
    Expected() = default;
    
    Expected(const T& value) : has_value_(true), value_(value) {}
    Expected(T&& value) : has_value_(true), value_(std::move(value)) {}
    
    Expected(const Unexpected<E>& error) : has_value_(false), error_(error.error()) {}
    Expected(Unexpected<E>&& error) : has_value_(false), error_(std::move(error.error())) {}
    
    template<typename U = T>
    Expected(std::in_place_t, U&& value) : has_value_(true), value_(std::forward<U>(value)) {}
    
    Expected(const Expected& other) : has_value_(other.has_value_) {
        if (other.has_value_) {
            new (&value_) T(other.value_);
        } else {
            new (&error_) E(other.error_);
        }
    }
    
    Expected(Expected&& other) noexcept : has_value_(other.has_value_) {
        if (other.has_value_) {
            new (&value_) T(std::move(other.value_));
        } else {
            new (&error_) E(std::move(other.error_));
        }
    }
    
    ~Expected() {
        if (has_value_) {
            value_.~T();
        } else {
            error_.~E();
        }
    }
    
    Expected& operator=(const Expected& other) {
        if (this != &other) {
            this->~Expected();
            new (this) Expected(other);
        }
        return *this;
    }
    
    Expected& operator=(Expected&& other) noexcept {
        if (this != &other) {
            this->~Expected();
            new (this) Expected(std::move(other));
        }
        return *this;
    }
    
    template<typename U = T>
    Expected& operator=(U&& value) {
        if (has_value_) {
            value_ = std::forward<U>(value);
        } else {
            error_.~E();
            new (&value_) T(std::forward<U>(value));
            has_value_ = true;
        }
        return *this;
    }
    
    Expected& operator=(const Unexpected<E>& error) {
        if (has_value_) {
            value_.~T();
            new (&error_) E(error.error());
        } else {
            error_ = error.error();
        }
        has_value_ = false;
        return *this;
    }
    
    Expected& operator=(Unexpected<E>&& error) {
        if (has_value_) {
            value_.~T();
            new (&error_) E(std::move(error.error()));
        } else {
            error_ = std::move(error.error());
        }
        has_value_ = false;
        return *this;
    }
    
    [[nodiscard]] bool has_value() const noexcept { return has_value_; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value_; }
    
    [[nodiscard]] T& value() & {
        if (!has_value_) throw std::runtime_error("Expected has no value");
        return value_;
    }
    
    [[nodiscard]] const T& value() const& {
        if (!has_value_) throw std::runtime_error("Expected has no value");
        return value_;
    }
    
    [[nodiscard]] T&& value() && {
        if (!has_value_) throw std::runtime_error("Expected has no value");
        return std::move(value_);
    }
    
    [[nodiscard]] const T&& value() const&& {
        if (!has_value_) throw std::runtime_error("Expected has no value");
        return std::move(value_);
    }
    
    [[nodiscard]] E& error() & {
        if (has_value_) throw std::runtime_error("Expected has value");
        return error_;
    }
    
    [[nodiscard]] const E& error() const& {
        if (has_value_) throw std::runtime_error("Expected has value");
        return error_;
    }
    
    [[nodiscard]] E&& error() && {
        if (has_value_) throw std::runtime_error("Expected has value");
        return std::move(error_);
    }
    
    [[nodiscard]] const E&& error() const&& {
        if (has_value_) throw std::runtime_error("Expected has value");
        return std::move(error_);
    }
    
    [[nodiscard]] T& operator*() & { return value(); }
    [[nodiscard]] const T& operator*() const& { return value(); }
    [[nodiscard]] T&& operator*() && { return value(); }
    [[nodiscard]] const T&& operator*() const&& { return value(); }
    
    [[nodiscard]] T* operator->() { return &value(); }
    [[nodiscard]] const T* operator->() const { return &value(); }
    
    template<typename U>
    [[nodiscard]] T value_or(U&& default_value) const& {
        return has_value_ ? value_ : static_cast<T>(std::forward<U>(default_value));
    }
    
    template<typename U>
    [[nodiscard]] T value_or(U&& default_value) && {
        return has_value_ ? std::move(value_) : static_cast<T>(std::forward<U>(default_value));
    }
    
    template<typename F>
    [[nodiscard]] auto and_then(F&& f) & {
        using ResultType = std::invoke_result_t<F, T&>;
        if (!has_value_) return ResultType(make_unexpected(error_));
        return std::invoke(std::forward<F>(f), value_);
    }
    
    template<typename F>
    [[nodiscard]] auto and_then(F&& f) && {
        using ResultType = std::invoke_result_t<F, T&&>;
        if (!has_value_) return ResultType(make_unexpected(std::move(error_)));
        return std::invoke(std::forward<F>(f), std::move(value_));
    }
    
    template<typename F>
    [[nodiscard]] auto transform(F&& f) & {
        using ResultType = std::invoke_result_t<F, T&>;
        if (!has_value_) return ResultType(make_unexpected(error_));
        return ResultType(std::invoke(std::forward<F>(f), value_));
    }
    
    template<typename F>
    [[nodiscard]] auto transform(F&& f) && {
        using ResultType = std::invoke_result_t<F, T&&>;
        if (!has_value_) return ResultType(make_unexpected(std::move(error_)));
        return ResultType(std::invoke(std::forward<F>(f), std::move(value_)));
    }
    
    template<typename F>
    [[nodiscard]] auto transform_error(F&& f) & {
        using NewError = std::invoke_result_t<F, E&>;
        if (has_value_) return Expected<T, NewError>(value_);
        return Expected<T, NewError>(make_unexpected(std::invoke(std::forward<F>(f), error_)));
    }
    
    template<typename F>
    [[nodiscard]] auto transform_error(F&& f) && {
        using NewError = std::invoke_result_t<F, E&&>;
        if (has_value_) return Expected<T, NewError>(std::move(value_));
        return Expected<T, NewError>(make_unexpected(std::invoke(std::forward<F>(f), std::move(error_))));
    }
    
    template<typename F>
    [[nodiscard]] auto or_else(F&& f) & {
        if (has_value_) return *this;
        return std::invoke(std::forward<F>(f), error_);
    }
    
    template<typename F>
    [[nodiscard]] auto or_else(F&& f) && {
        if (has_value_) return std::move(*this);
        return std::invoke(std::forward<F>(f), std::move(error_));
    }

private:
    union {
        T value_;
        E error_;
    };
    bool has_value_ = false;
};

template<typename E>
class Expected<void, E> {
public:
    Expected() : has_value_(true) {}
    
    Expected(std::in_place_t) : has_value_(true) {}
    
    Expected(const Unexpected<E>& error) : has_value_(false), error_(error.error()) {}
    Expected(Unexpected<E>&& error) : has_value_(false), error_(std::move(error.error())) {}
    
    Expected(const Expected& other) : has_value_(other.has_value_) {
        if (!other.has_value_) {
            new (&error_) E(other.error_);
        }
    }
    
    Expected(Expected&& other) noexcept : has_value_(other.has_value_) {
        if (!other.has_value_) {
            new (&error_) E(std::move(other.error_));
        }
    }
    
    ~Expected() {
        if (!has_value_) {
            error_.~E();
        }
    }
    
    Expected& operator=(const Expected& other) {
        if (this != &other) {
            this->~Expected();
            new (this) Expected(other);
        }
        return *this;
    }
    
    Expected& operator=(Expected&& other) noexcept {
        if (this != &other) {
            this->~Expected();
            new (this) Expected(std::move(other));
        }
        return *this;
    }
    
    Expected& operator=(const Unexpected<E>& error) {
        if (!has_value_) {
            error_ = error.error();
        } else {
            new (&error_) E(error.error());
        }
        has_value_ = false;
        return *this;
    }
    
    Expected& operator=(Unexpected<E>&& error) {
        if (!has_value_) {
            error_ = std::move(error.error());
        } else {
            new (&error_) E(std::move(error.error()));
        }
        has_value_ = false;
        return *this;
    }
    
    [[nodiscard]] bool has_value() const noexcept { return has_value_; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value_; }
    
    void value() const {
        if (!has_value_) throw std::runtime_error("Expected has no value");
    }
    
    [[nodiscard]] E& error() & {
        if (has_value_) throw std::runtime_error("Expected has value");
        return error_;
    }
    
    [[nodiscard]] const E& error() const& {
        if (has_value_) throw std::runtime_error("Expected has value");
        return error_;
    }
    
    [[nodiscard]] E&& error() && {
        if (has_value_) throw std::runtime_error("Expected has value");
        return std::move(error_);
    }
    
    [[nodiscard]] const E&& error() const&& {
        if (has_value_) throw std::runtime_error("Expected has value");
        return std::move(error_);
    }
    
    template<typename F>
    [[nodiscard]] auto and_then(F&& f) & {
        using ResultType = std::invoke_result_t<F>;
        if (!has_value_) return ResultType(make_unexpected(error_));
        return std::invoke(std::forward<F>(f));
    }
    
    template<typename F>
    [[nodiscard]] auto and_then(F&& f) && {
        using ResultType = std::invoke_result_t<F>;
        if (!has_value_) return ResultType(make_unexpected(std::move(error_)));
        return std::invoke(std::forward<F>(f));
    }
    
    template<typename F>
    [[nodiscard]] auto transform(F&& f) & {
        using ResultType = std::invoke_result_t<F>;
        if (!has_value_) return ResultType(make_unexpected(error_));
        return ResultType(std::invoke(std::forward<F>(f)));
    }
    
    template<typename F>
    [[nodiscard]] auto transform(F&& f) && {
        using ResultType = std::invoke_result_t<F>;
        if (!has_value_) return ResultType(make_unexpected(std::move(error_)));
        return ResultType(std::invoke(std::forward<F>(f)));
    }
    
    template<typename F>
    [[nodiscard]] auto transform_error(F&& f) & {
        using NewError = std::invoke_result_t<F, E&>;
        if (has_value_) return Expected<void, NewError>();
        return Expected<void, NewError>(make_unexpected(std::invoke(std::forward<F>(f), error_)));
    }
    
    template<typename F>
    [[nodiscard]] auto transform_error(F&& f) && {
        using NewError = std::invoke_result_t<F, E&&>;
        if (has_value_) return Expected<void, NewError>();
        return Expected<void, NewError>(make_unexpected(std::invoke(std::forward<F>(f), std::move(error_))));
    }
    
    template<typename F>
    [[nodiscard]] auto or_else(F&& f) & {
        if (has_value_) return *this;
        return std::invoke(std::forward<F>(f), error_);
    }
    
    template<typename F>
    [[nodiscard]] auto or_else(F&& f) && {
        if (has_value_) return std::move(*this);
        return std::invoke(std::forward<F>(f), std::move(error_));
    }

private:
    E error_;
    bool has_value_ = true;
};

} // namespace game_req