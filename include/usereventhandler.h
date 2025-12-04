#ifndef USER_EVENT_HANDLER_H
#define USER_EVENT_HANDLER_H

#include "ieventhandler.h"
#include "types.h"
#include "ievent.h"//因为function是类型擦除的模板类，所以只向前声明不行
#include "userdao.h"

#include <unordered_map>
#include <mutex>
#include <functional>
#include <memory>

class MobileCodeReqEv;
class MobileCodeRspEv;
class LoginReq;
class LoginRsp;
class DispatchMsgService;

class UserEventHandler : public iEventHandler {
public:
  virtual void handle(std::unique_ptr<iEvent> ev);
  explicit UserEventHandler(DispatchMsgService* dms);
  virtual ~UserEventHandler();
private:
//使用不同的用户端私有函数
  MobileCodeRspEv* handle_mobile_code_req(MobileCodeReqEv* ev);
  LoginRsp* handle_login_req(LoginReq* ev);

//使用事件注册表+函数分发代替switchcase
  std::unordered_map<u32, std::function<iEvent*(iEvent*)>> dispatcher_;
//我们自己写一个获取验证码的函数
  i32 gen_icode();
  
  std::unique_ptr<UserDao> user_dao_;

  DispatchMsgService* dms_;

};

#endif