#ifndef LOGIN_REQ_H
#define LOGIN_REQ_H

#include "ievent.h"
#include "bike.pb.h"
#include <string>

class LoginReq : public iEvent {
public:
  LoginReq(const std::string& mobile, i32 icode);
  virtual std::ostream& dump(std::ostream& out) override;
  const std::string& get_mobile() const {
    return msg_.mobile();
  }
  i32 get_icode() const {
    return msg_.icode();
  }
private:
  tutorial::login_request msg_;
};

#endif