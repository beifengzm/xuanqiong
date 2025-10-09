#pragma once

#include <coroutine>

namespace xuanqiong {

template<typename Promise>
struct Task {
    using promise_type = Promise;
    using HandleType = std::coroutine_handle<Promise>;

    Task(HandleType handle) : handle_(handle) {}

private:
    HandleType handle_;
};

} // namespace xuanqiong
