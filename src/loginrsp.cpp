#include "loginrsp.h"
#include "errormanager.h"
#include "spdlog/spdlog.h"

LoginRsp::LoginRsp(i32 code) : iEvent(EEVENTID_LOGIN_RSP, generateSeqNo()) {
  msg_.set_code(code);
  auto desc = ErrorManager::getInstance().getDesc(code);
  if (desc.has_value()) {
    msg_.set_desc(desc.value());
  }
}

std::ostream& LoginRsp::dump(std::ostream& out) {
  out << "[LoginRsp] Code: " << msg_.code() << ", Desc: " << msg_.desc();
  return out;
}

std::string LoginRsp::serialize() {
  std::string body;
  if (!msg_.SerializeToString(&body)) {
    spdlog::error("[LoginRsp] SerializeToString failed");
    return "";
  }
  return body;
}