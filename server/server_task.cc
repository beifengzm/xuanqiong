#include "server/server_task.h"

#include "server/server_task.h"

#include "util/common.h"
#include "util/iobuf.h"
#include "net/socket.h"
#include "scheduler/scheduler.h"

namespace xuanqiong::server {

void ServerTask::run() {
    while (true) {
        int nread = co_await socket_->read_data();
        if (nread == -1) {
            error("error occurred in read: %s", strerror(errno));
            continue;
        }
    }
}

} // namespace xuanqiong::server
