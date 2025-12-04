#ifndef IEVENT_H
#define IEVENT_H

#include "types.h"
#include "eventtype.h"
#include "event2/bufferevent.h"
#include <string>
#include <iostream>

class iEvent {
public:
  iEvent(u32 eid, u32 sn);//eid是为了确定请求类型，sn是为了区分相同类型的不同次请求
  virtual std::ostream& dump(std::ostream& out);
  virtual ~iEvent();
  virtual std::string serialize() {return "";};

  void set_fd(int fd) {
    fd_ = fd;
  }
  int get_fd() const {
    return fd_;
  }

  u32 generateSeqNo();
  u32 getEid() const;
  u32 getSn() const;
private:
  u32 eid_;
  u32 sn_;
  int fd_;
};
#endif