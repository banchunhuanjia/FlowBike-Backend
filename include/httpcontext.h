#ifndef HTTP_CONTEXT_H
#define HTTP_CONTEXT_H

#include <string>
#include "httprequest.h"

enum CheckState {
  CHECK_STATE_REQUESTLINE = 0,
  CHECK_STATE_HEADER,
  CHECK_STATE_CONTENT,
};

// 解析结果代码，告诉下一层要做什么
enum HttpCode {
  NO_REQUEST, // 数据不完整，继续读
  GET_REQUEST, // 解析完成，有一个完整请求
  BAD_REQUEST, // 报文语法错误
  INTERNAL_ERROR,
  CLOSED_CONNECTION,
};

class HttpContext {
public:
  HttpContext() {
    state_ = CHECK_STATE_REQUESTLINE;
    m_idx_ = 0;
  }

  HttpCode parse(char* buffer, int size);

  HttpRequest& getRequest() { return request_; }
  size_t getParseLen() const { return m_idx_; }
  
  void reset() {
    state_ = CHECK_STATE_REQUESTLINE;
    HttpRequest dummy;
    request_ = dummy;
    m_idx_ = 0;
  } 
private:
  bool parseRequestLine(const std::string& line);
  bool parseHeaders(const std::string& line);
  bool parseBody(const std::string& line);

  HttpRequest request_;
  CheckState state_;

  int m_idx_; // 记录解析到第几个字符，我们选择了内部索引法
};

#endif