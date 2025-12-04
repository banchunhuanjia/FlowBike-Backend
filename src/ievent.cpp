#include "ievent.h"

iEvent::iEvent(u32 eid, u32 sn) : eid_(eid), sn_(sn), fd_(-1) {}
iEvent::~iEvent() {}

std::ostream& iEvent::dump(std::ostream& out) {
  return out;
}

u32 iEvent::generateSeqNo() {
  static u32 sn = 0;
  return sn++;
}

u32 iEvent::getEid() const {
  return eid_;
}

u32 iEvent::getSn() const {
  return sn_;
}
