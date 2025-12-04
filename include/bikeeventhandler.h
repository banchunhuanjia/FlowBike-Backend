#ifndef BIKE_EVENT_HANDLER_H
#define BIKE_EVENT_HANDLER_H

#include "ieventhandler.h"
#include "types.h"
#include "ievent.h"

#include <unordered_map>
#include <functional>
#include <memory>

class UnlockReq;
class UnlockRsp;
class DispatchMsgService;
class UserDao;
class BikeDao;

class BikeEventHandler : public iEventHandler {
public:
  virtual void handle(std::unique_ptr<iEvent> ev);
  explicit BikeEventHandler(DispatchMsgService* dms);
  virtual ~BikeEventHandler();
private:
  UnlockRsp* handle_unlock_req(UnlockReq* ev);

  std::unordered_map<u32, std::function<iEvent*(iEvent*)>> dispatcher_;

  std::unique_ptr<BikeDao> bike_dao_;
  std::unique_ptr<UserDao> user_dao_;

  DispatchMsgService* dms_;
};

#endif