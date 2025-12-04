#ifndef MOBILE_CODE_REQ_EV_H
#define MOBILE_CODE_REQ_EV_H

#include "ievent.h"
#include "bike.pb.h"
#include <string>

class MobileCodeReqEv : public iEvent {
public:
  MobileCodeReqEv(const std::string& mobile);
  const std::string& get_mobile() const ;
  virtual std::ostream& dump(std::ostream& out) override;
private:
  tutorial::mobile_request msg_;
};

#endif