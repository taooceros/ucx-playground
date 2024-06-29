
#include <coroutine>
#include <cstddef>
#include <optional>

template <typename T> class ucp_task {
    std::coroutine_handle<> handle;
    std::optional<T> result;

    typedef void (*callback)(T user_data, void *arg);

  public:
    static void register_callback(T user_data, void *arg) {
        auto task = (ucp_task<T> *)arg;
        task->result = user_data;
        if (task->handle)
            task->handle.resume();
    }

    auto operator co_await() {
        struct awaiter {
            ucp_task<T> &task;
            bool await_ready() { return task.result.has_value(); }
            void await_suspend(std::coroutine_handle<> h) { task.handle = h; }
            T await_resume() { return task.result.value(); }
        };
        return awaiter{*this};
    }
};