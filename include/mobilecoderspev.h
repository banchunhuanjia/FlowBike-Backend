#ifndef MOBILE_CODE_RSP_EV_H
#define MOBILE_CODE_RSP_EV_H

#include "ievent.h"
#include "bike.pb.h"
#include <string>
#include <optional>


class MobileCodeRspEv : public iEvent {
public:
  MobileCodeRspEv(i32 code, i32 icode);
  virtual std::ostream& dump(std::ostream& out) override;
  virtual std::string serialize()  override;

  i32 getCode() const;
  i32 getiCode() const;
  std::optional<std::string> getDesc() const;
private:
  tutorial::mobile_response msg_;
};

#endif