#include "unlockrsp.h"
#include "errormanager.h"
#include "spdlog/spdlog.h"

UnlockRsp::UnlockRsp(i32 code) : iEvent(EEVENTID_UNLOCK_RSP, generateSeqNo()) {
  msg_.set_code(code);
  auto desc = ErrorManager::getInstance().getDesc(code);
  if (desc.has_value()) {
    msg_.set_desc(desc.value());
  }
}

std::ostream& UnlockRsp::dump(std::ostream& out) {
    out << "[UnlockRsp] Code: " << msg_.code() << ", Desc: " << msg_.desc();
    return out;
}

std::string UnlockRsp::serialize() {
  std::string body;
  if (!msg_.SerializeToString(&body)) {
    spdlog::error("[UnlockRsp] SerializeToString failed");
    return "";
  }
  return body;
}