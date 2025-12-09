#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>

enum Method { GET = 0, POST, HEAD, PUT, DELETE, UNKNOWN };
enum Version { HTTP_10, HTTP_11, OTHER_VERSION };

class HttpRequest {
public:
  HttpRequest() {
    method_ = GET;
    version_ = HTTP_11;
  }


  void setMethod(const std::string& method) {
    if (method == "GET") method_ = GET;
    else if (method == "POST") method_ = POST;
    else if (method == "HEAD") method_ = HEAD;
    else if (method == "PUT") method_ = PUT;
    else if (method == "DELETE") method_ = DELETE;
    else method_ = UNKNOWN;
  }
  void setUrl(const std::string& url) { url_ = url; }
  void setVersion(const std::string& version) {
    if (version == "HTTP/1.1") version_ = HTTP_11;
    else version_ = HTTP_10;
  }
  void setBody(const std::string& body) { body_ = body; }
  void addHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
  }

  Method getMethod() const { return method_; }
  Version getVersion() const { return version_; }
  std::string getPath() const { return url_; }
  std::string getHeader(const std::string& key) const {
    if (headers_.count(key)) {
      return headers_.at(key);
    }
    return "";
  }
  std::string getBody() const { return body_; } 


private:
  Method method_;
  Version version_;
  std::string url_;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
};


#endif 