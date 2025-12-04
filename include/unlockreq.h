#ifndef UNLOCK_REQ_H
#define UNLOCK_REQ_H

#include "ievent.h"
#include "bike.pb.h"
#include <string>

class UnlockReq : public iEvent {
public:
  UnlockReq(const std::string& mobile, i32 dev_no, const std::string& token);

  virtual std::ostream& dump(std::ostream& out) override;

  const std::string& getMobile() const {
    return msg_.mobile();
  }
  i32 getDevNo() const {
    return msg_.dev_no();
  }
  std::string getToken() const {
    return msg_.token();
  }
private:
  tutorial::unlock_request msg_;
};

#endif