#include "loginreq.h"

LoginReq::LoginReq(const std::string& mobile, i32 icode) : iEvent(EEVENTID_LOGIN_REQ, generateSeqNo()) {
  msg_.set_mobile(mobile);
  msg_.set_icode(icode);
}

std::ostream& LoginReq::dump(std::ostream& out) {
  out << "[LoginReq] Mobile: " << msg_.mobile() << ", Code: " << msg_.icode();
  return out;
}
