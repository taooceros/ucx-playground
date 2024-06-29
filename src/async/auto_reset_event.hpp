#pragma once

#include <atomic>
#include <coroutine>
#include <functional>
#include <optional>

namespace async {

template <typename T> class async_auto_reset_event_operation;

template <typename T> class async_auto_reset_event {
  public:
    async_auto_reset_event(bool signaled = false) : signaled_(signaled) {}

    async_auto_reset_event_operation<T> operator co_await() noexcept {
        return async_auto_reset_event_operation(*this);
    }

    void set_or(T data,
                std::function<void()> full_callback = nullptr) noexcept {
        if (signaled_) {
            if (full_callback)
                full_callback();
            return;
        }
        this->data = data;
        signaled_ = true;
        if (waiter_handle) {
            waiter_handle.resume();
            waiter_handle = nullptr;
        }
    }

    void reset() noexcept { signaled_ = false; }

  private:
    std::atomic<bool> signaled_;
    std::optional<T> data;
    std::coroutine_handle<> waiter_handle;

    friend class async_auto_reset_event_operation<T>;
};

template <typename T> class async_auto_reset_event_operation {
  public:
    async_auto_reset_event_operation() noexcept = default;

    explicit async_auto_reset_event_operation(
        async_auto_reset_event<T> &event) noexcept {
        m_event = &event;
    }

    async_auto_reset_event_operation(
        const async_auto_reset_event_operation &other) noexcept = default;

    bool await_ready() const noexcept { return m_event->signaled_; }
    bool await_suspend(std::coroutine_handle<> awaiter) noexcept {
        if (!m_event->signaled_) {
            m_event->waiter_handle = awaiter;
            return true;
        }
        return !m_event->signaled_;
    }
    T await_resume() const noexcept { return m_event->data.value(); }

  private:
    friend class async_auto_reset_event<T>;

    async_auto_reset_event<T> *m_event;
};

} // namespace async