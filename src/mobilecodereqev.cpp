#include "mobilecodereqev.h"

MobileCodeReqEv::MobileCodeReqEv(const std::string& mobile) : iEvent(EEVENTID_GET_MOBILE_CODE_REQ, generateSeqNo()) {
  msg_.set_mobile(mobile);
}

const std::string& MobileCodeReqEv::get_mobile() const {
  return msg_.mobile();
}//这个一定在返回值加const不然和msg_.mobile()匹配不上

std::ostream& MobileCodeReqEv::dump(std::ostream& out) {
  out << "MobileCodeReq sn = " << getSn() << ",";
  out << "mobile = " << msg_.mobile() << std::endl;
  return out;
}