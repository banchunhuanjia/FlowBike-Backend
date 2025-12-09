#include "httpcontext.h"

#include <sstream>
#include <algorithm> //为了search函数

HttpCode HttpContext::parse(char* buffer, int size) {
  size_t next_pos = 0;

  while (state_ != CHECK_STATE_CONTENT) {
    char* text_begin = buffer;
    char* text_end = buffer + size;

    char* line_end_ptr = std::search(text_begin + m_idx_, text_end, "\r\n", "\r\n" + 2);
    if (line_end_ptr == text_end) {
      return NO_REQUEST;
    }
    next_pos = line_end_ptr - text_begin;
    std::string line(text_begin + m_idx_, line_end_ptr);

    m_idx_ = next_pos + 2; // 跳过 \r\n
    switch(state_) {
      case CHECK_STATE_REQUESTLINE: {
        if (!parseRequestLine(line)) {
          return BAD_REQUEST;
        }
        state_ = CHECK_STATE_HEADER;
        break;
      }
      case CHECK_STATE_HEADER: {
        // 空行说明HEADER结束，改到BODY了
        if (line.empty()) {
          state_ = CHECK_STATE_CONTENT;
        } else {
          if (!parseHeaders(line)) {
            return BAD_REQUEST;
          }
        } 
        break;
      }
      default: {
        break;
      }
    }
  }
  if (state_ == CHECK_STATE_CONTENT) {
    // 剩下的全是BODY
    std::string body_content(buffer + m_idx_, buffer + size);
    if (!parseBody(body_content)) {
      return NO_REQUEST;
    }
      return GET_REQUEST;
  }
  return NO_REQUEST;
}

bool HttpContext::parseRequestLine(const std::string& line) {
  std::stringstream ss(line);
  std::string method, url, version;
  if (!(ss >> method >> url >> version)) return false;
  request_.setMethod(method);
  request_.setUrl(url);
  request_.setVersion(version);
  return true;
}

bool HttpContext::parseHeaders(const std::string& line) {
  size_t colon_pos = line.find(':');
  if (colon_pos == std::string::npos) return false;
  std::string key = line.substr(0, colon_pos);
  std::string value = line.substr(colon_pos + 1);

  //去除前导空格(因为可以只有冒号，也可以好多空格)
  size_t first_not_space = value.find_first_not_of(' ');
  if (first_not_space != std::string::npos) {
    value = value.substr(first_not_space);
  }
  request_.addHeader(key, value);
  return true;
}

bool HttpContext::parseBody(const std::string& line) {
  std::string content_len_str = request_.getHeader("Content-Length");
  if (!content_len_str.empty()) {
    int len = std::stoi(content_len_str);
    if (line.size() < len) {
      return false;
    }
    request_.setBody(line.substr(0, len));
  } else {
    // 如果没有Content-Length通常body为空或者全读
    request_.setBody(line);
  }
  return true;
}