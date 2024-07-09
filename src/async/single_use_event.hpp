#pragma once

#include "fmt/core.h"
#include <atomic>
#include <coroutine>
#include <functional>
#include <optional>

namespace async {

template <typename T> class single_use_event_operation;

template <typename T> class single_use_event {
  public:
    single_use_event(bool initial_signaled = false)
        : signaled_(initial_signaled) {}

    single_use_event_operation<T> operator co_await() noexcept {
        return single_use_event_operation(*this);
    }

    template <typename Callback = std::function<void(T)>>
    void set(T data) noexcept {
        this->data = data;
        signaled_ = true;
        if (waiter_handle) {
            waiter_handle.resume();
        }
    }

    void reset() noexcept { signaled_ = false; }

  private:
    std::atomic<bool> signaled_;
    std::optional<T> data;
    std::coroutine_handle<> waiter_handle;

    friend class single_use_event_operation<T>;
};

template <typename T> class single_use_event_operation {
  public:
    single_use_event_operation() noexcept = default;

    explicit single_use_event_operation(single_use_event<T> &event) noexcept {
        m_event = &event;
    }

    single_use_event_operation(
        const single_use_event_operation &other) noexcept = default;

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
    friend class single_use_event<T>;

    single_use_event<T> *m_event;
};

} // namespace async