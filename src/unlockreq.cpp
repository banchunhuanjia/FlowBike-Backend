#include "unlockreq.h"

UnlockReq::UnlockReq(const std::string& mobile, i32 dev_no, const std::string& token) : iEvent(EEVENTID_UNLOCK_REQ, generateSeqNo()) {
  msg_.set_mobile(mobile);
  msg_.set_dev_no(dev_no);
  msg_.set_token(token);
}

std::ostream& UnlockReq::dump(std::ostream& out) {
  out << "[UnlockReq] Mobile: " << msg_.mobile() << ", DevNo: " << msg_.dev_no();
  return out;
}