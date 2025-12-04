#ifndef EVENT_TYPE_H
#define EVENT_TYPE_H

enum EventID {
  EEVENTID_GET_MOBILE_CODE_REQ = 0x01,
  EEVENTID_GET_MOBILE_CODE_RSP = 0x02,

  EEVENTID_LOGIN_REQ = 0x03,
  EEVENTID_LOGIN_RSP = 0x04,

  EEVENTID_UNLOCK_REQ = 0x05,
  EEVENTID_UNLOCK_RSP = 0x06,

  EEVENTID_UNKOWN = 0xFF
//还可以添加其他事件
};

enum EErrorCode {
  ERRC_SUCCESS = 0,
  // --- General Errors (1-999) ---
  ERRC_INVALID_MSG = 1,
  ERRC_INVALID_DATA = 2,
  ERRC_METHOD_NOT_ALLOWED = 3,
  ERRO_PROCESS_FAILED = 4,
  // --- Bike Specific Errors (1000-1999) ---
  ERRO_BIKE_IS_TAKEN = 1000,
  ERRO_BIKE_IS_RUNNING = 1001,
  ERRO_BIKE_IS_DAMAGED = 1002,

  // --- HTTP Mapping (for external reference, not in code logic) ---
  // ERRC_SUCCESS -> HTTP 200
  // ERRC_INVALID_MSG -> HTTP 400
  // ERRC_INVALID_DATA -> HTTP 404
  // etc.
};

//对于我们这种平常项目这两个不用分离

#endif

