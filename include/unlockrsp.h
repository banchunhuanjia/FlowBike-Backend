#ifndef UNLOCK_RSP_H
#define UNLOCK_RSP_H

#include "ievent.h"
#include "bike.pb.h"
#include <string>

class UnlockRsp : public iEvent {
public:
  UnlockRsp(i32 code);
  virtual std::ostream& dump(std::ostream& out) override;
  virtual std::string serialize() override;
private:
  tutorial::unlock_response msg_;
};

#endif