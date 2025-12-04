#ifndef IEVENT_HANDLER_H
#define IEVENT_HANDLER_H

#include <memory>

class iEvent;
class iEventHandler {
public:
  //纯虚函数，没有父类实例
  virtual void handle(std::unique_ptr<iEvent> ev) = 0;
  virtual ~iEventHandler() = default;
protected:
  iEventHandler() = default;
};

#endif