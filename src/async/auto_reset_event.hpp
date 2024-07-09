#pragma once

#include "fmt/core.h"
#include <atomic>
#include <coroutine>
#include <functional>
#include <memory>
#include <optional>

namespace async {

template <typename T> class auto_reset_event_handle {
  public:
    auto_reset_event_handle() noexcept {
        m_event = std::make_shared<auto_reset_event>();
    }

    class auto_reset_event {
      public:
        auto_reset_event(bool initial_signaled = false)
            : signaled_(initial_signaled) {}

        template <typename Callback = std::function<void(T)>>
        void set_or(T data, Callback full_callback = nullptr) noexcept {
            if (signaled_) {
                if (full_callback)
                    full_callback(data);
                return;
            }
            this->data = data;
            signaled_ = true;
            if (waiter_handle) {
                waiter_handle.resume();
                waiter_handle = nullptr;
                signaled_ = false;
            }
        }

        void reset() noexcept { signaled_ = false; }

      private:
        std::atomic<bool> signaled_;
        std::optional<T> data;
        std::coroutine_handle<> waiter_handle;

        friend class auto_reset_event_handle;
    };

    class auto_reset_event_operation {
      public:
        auto_reset_event_operation() noexcept = default;

        explicit auto_reset_event_operation(auto_reset_event &event) noexcept {
            m_event = &event;
        }

        auto_reset_event_operation(
            const auto_reset_event_operation &other) noexcept = default;

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
        auto_reset_event *m_event;
    };

    auto_reset_event_operation operator co_await() noexcept {
        return auto_reset_event_operation(*m_event);
    }

    template <typename Callback = std::function<void(T)>>
    void set_or(T data, Callback full_callback = nullptr) noexcept {
        m_event->set_or(data, full_callback);
    }

  private:
    std::shared_ptr<auto_reset_event> m_event;
};

} // namespace async