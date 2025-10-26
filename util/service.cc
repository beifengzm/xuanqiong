#include "util/service.h"

namespace xuanqiong {

void RpcController::Reset() {
  failed_ = false;
  canceled_ = false;
  error_text_.clear();
  timeout_ms_ = -1;
}

void RpcController::StartCancel() {
  canceled_ = true;
}

void RpcController::NotifyOnCancel(google::protobuf::Closure* callback) {
  
}

void RpcController::SetFailed(const std::string& reason) {
  failed_ = true;
  error_text_ = reason;
}

}  // namespace xuanqiong
