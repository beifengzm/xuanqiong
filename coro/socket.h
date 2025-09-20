#pragma once

#include <coroutine>
#include <unistd.h>
#include <sys/socket.h>

#include "base/common.h"
#include "base/iobuf.h"

namespace xuanqiong::coro {

struct ReadAwaiter {
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        // 注册回调函数，当数据可读时调用handle.resume()
        info("read await suspend");
    }
    void await_resume() {}
};

class Socket {
    ReadAwaiter read_data() {
        // while (true) {
        //     int read_num = read(sockfd_, &buf_, sizeof(buf_));
        //     if (read_num > 0) {
        //         // 数据读取成功，处理数据
        //     } else if (read_num == 0) {
        //         // 对端关闭连接
        //         break;
        //     } else {
        //         // 读取错误，处理错误
        //         break;
        //     }
        // }
        return {};
    }

private:
    int sockfd_;
    base::IOBuf buf_;
};

} // namespace xuanqiong::coro
