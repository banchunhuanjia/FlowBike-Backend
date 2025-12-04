#include "mobilecoderspev.h"
#include "errormanager.h"
#include "spdlog/spdlog.h"

MobileCodeRspEv::MobileCodeRspEv(i32 code, i32 icode) :iEvent(EEVENTID_GET_MOBILE_CODE_RSP, generateSeqNo()) {
  msg_.set_code(code);
  msg_.set_icode(icode);
  auto desc_opt = ErrorManager::getInstance().getDesc(code);
  if (desc_opt.has_value()) {
    msg_.set_data(desc_opt.value());
  }
}

std::ostream& MobileCodeRspEv::dump(std::ostream& out) {
  out << "MobileCodeRsp sn = " << getSn() << ",";
  out << "code = " << msg_.code() <<", icode = " << msg_.icode() << std::endl;
  out << "desc = " << msg_.data() << std::endl;
  return out;
}

i32 MobileCodeRspEv::getCode() const {
  return msg_.code();
}

i32 MobileCodeRspEv::getiCode() const {
  return msg_.icode();
}

std::optional<std::string> MobileCodeRspEv::getDesc() const {
  if (msg_.has_data()) {
    return msg_.data();//使用proto自己的判别optional
  } else {
    return std::nullopt;
  }
}

std::string MobileCodeRspEv::serialize() {
  std::string body;
  if (!msg_.SerializeToString(&body)) {
    spdlog::error("[MobileCodeRspEv] SerializeToString failed");
    return "";
  }
  return body;
}