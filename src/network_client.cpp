#include "network_client.h"
#include "spdlog/spdlog.h"
#include "eventtype.h"
#include "bike.pb.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>


NetworkClient::NetworkClient(const std::string& ip, int port)
   : ip_(ip), port_(port), sock_(-1), is_connected_(false){}

NetworkClient::~NetworkClient() {
  Close();
}

bool NetworkClient::start() {
  is_connected_ = true;
  sock_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_ < 0) {
    spdlog::error("[Client] Create socket failed");
    return false;
  }
  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_);
  if (inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
    spdlog::error("[Client] Invalid address / Addresss not supported");
    return false;
  }
  if (connect(sock_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    spdlog::error("[Client] Connection failed");
    return false;
  }
  return true;
}

bool NetworkClient::sendRequest(u32 code, const std::string& body) {
  if (!is_connected_) return false;

  PacketHeader header;
  header.length = htonl(body.size());
  header.type_id = htonl(code);

  if (send(sock_, &header, sizeof(header), 0) != sizeof(header)) {
    spdlog::error("[Client] Write header failed");
    Close();
    return false;
  }
  if (send(sock_, body.data(), body.size(), 0) != (ssize_t)body.size()) {
    spdlog::error("[Client] Write body failed");
    Close();
    return false;
  }
  return true;
}

bool NetworkClient::getResponse(int& out_msg_code, std::string& out_data) {
  if (!is_connected_) return false;

  PacketHeader rsp_header{};
  ssize_t n = read(sock_, &rsp_header, sizeof(rsp_header));
  if (n <= 0) {
    spdlog::error("[Client] Server closed connection");
    Close();
    return false;
  }
  // 这个转换到host不能忘了
  rsp_header.length = ntohl(rsp_header.length);
  rsp_header.type_id = ntohl(rsp_header.type_id);

  std::string rsp_body(rsp_header.length, '\0');
  size_t read_bytes = 0;
  while (read_bytes < rsp_header.length) {
    ssize_t r = read(sock_, &rsp_body[read_bytes], rsp_header.length - read_bytes);
    if (r <= 0) {
      spdlog::error("[Client] Read body failed");
      Close();
      return false;
    }
    read_bytes += r;
  }

  out_msg_code = -1;
  out_data = "";

  if (rsp_header.type_id == EEVENTID_GET_MOBILE_CODE_RSP) {
    tutorial::mobile_response rsp;
    if (rsp.ParseFromString(rsp_body)) {
      spdlog::debug("============ Server Response =========");
      spdlog::debug("Code: {}", rsp.code());
      spdlog::debug("ICode: {}", rsp.icode());
      if (rsp.has_data()) {
        spdlog::debug("Data: {}", rsp.data());
      }
      spdlog::debug("======================================");
      out_msg_code = rsp.code();
      if (rsp.has_data()) {
        out_data = rsp.data();
      }
      return true;
    }

  } else if (rsp_header.type_id == EEVENTID_LOGIN_RSP) {
    tutorial::login_response rsp;
    if (rsp.ParseFromString(rsp_body)) {
      spdlog::debug("============ Server Response =========");
      spdlog::debug("Code: {}", rsp.code());
      spdlog::debug("Token: {}", rsp.token());
      spdlog::debug("Desc: {}", rsp.desc());
      spdlog::debug("======================================");
      out_msg_code = rsp.code();
      if (rsp.code() == 0) {
        out_data = rsp.token();
      } else {
        out_data = rsp.desc();
      }
      return true;
    }
  } else if (rsp_header.type_id == EEVENTID_UNLOCK_RSP) {
    tutorial::unlock_response rsp;
    if (rsp.ParseFromString(rsp_body)) {
      spdlog::debug("============ Server Response =========");
      spdlog::debug("Code: {}", rsp.code());
      spdlog::debug("Desc: {}", rsp.desc());
      spdlog::debug("======================================");
      out_msg_code = rsp.code();
      out_data = rsp.desc();
      return true;
    }
  } else {
    spdlog::error("[Client] Unkown message type: {}", rsp_header.type_id);
    Close();
  }
  return false;
}

void NetworkClient::Close() {
  if (sock_ != -1) {
    close(sock_);
    sock_ = -1;
  }
  is_connected_ = false;
}