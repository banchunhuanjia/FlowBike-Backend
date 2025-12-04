#ifndef LOGIN_RSP_H
#define LOGIN_RSP_H

#include "ievent.h"
#include "bike.pb.h"
#include <string>

class LoginRsp : public iEvent {
public:
  LoginRsp(i32 code);
  virtual std::ostream& dump(std::ostream& out) override;
  virtual std::string serialize() override;
  void setToken(const std::string& token) {
    msg_.set_token(token);
  }
private:
  tutorial::login_response msg_;
};

#endif