#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "bike.pb.h"
#include "packetheader.h"
#include "eventtype.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/fmt/ostr.h"
#include "global_config.h"
#include "network_client.h"

void initLogger(const std::string& log_path, const std::string& level_str) {
  try {
    // 创建控制台输出
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    // 创建文件输出，用追加模式
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, false);
    file_sink->set_level(spdlog::level::info);
    // 组合两个输出地
    std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
    // 设置默认全局logger
    spdlog::set_default_logger(logger);
    // 根据配置文件字符串设置日志级别
    spdlog::level::level_enum level = spdlog::level::info;
    if (level_str == "debug") level = spdlog::level::debug;
    else if (level_str == "warn") level = spdlog::level::warn;
    else if (level_str == "error") level = spdlog::level::err;
    
    spdlog::set_level(level);
    // 遇到info级别立马刷新到磁盘，放丢失
    spdlog::flush_on(spdlog::level::info);
    // 不在缓存中攒着，3秒刷新一次并写入
    spdlog::flush_every(std::chrono::seconds(3));

  } catch (const spdlog::spdlog_ex& ex) {
    //日志系统挂了，只能标准输出
    std::cerr << "Log init failed: " << ex.what() << std::endl;
    exit(1);
  }
}

void cmd_get_mobile_code(NetworkClient& client) {
  tutorial::mobile_request req;
  std::string mobile;
  spdlog::info("> Enter Mobile Number:");
  std::cin >> mobile;
  req.set_mobile(mobile);
  
  std::string body;
  req.SerializeToString(&body);
  client.sendRequest(EEVENTID_GET_MOBILE_CODE_REQ, body);
  
  int code = 0;
  std::string data = "";
  if (client.getResponse(code, data)) {
    if (code == ERRC_SUCCESS) {
      spdlog::info("Verify code sent successfully! {}", data);
    } else {
      spdlog::error("Failed to get code. {}", data);
    }
  } else {
    spdlog::error("Network error.");
  }
}

// string的返回值是必要的，他要处理修改主函数的current_mobile
// current_mobile非常重要，它关系到我们的客户端显示
// 但是由于token的引入，我们采用返回token，用参数带出mobile的策略
std::string cmd_login(NetworkClient& client, std::string& out_mobile) {
  tutorial::login_request req;
  std::string mobile;
  int icode;
  spdlog::info("> Enter Mobile Number:");
  std::cin >> mobile; 
  spdlog::info("> Enter Icode:");
  std::cin >> icode;

  req.set_mobile(mobile);
  req.set_icode(icode);
      
  std::string body;
  req.SerializeToString(&body);
  client.sendRequest(EEVENTID_LOGIN_REQ, body);

  int code = 0;
  std::string data = "";
  if (client.getResponse(code, data)) {
    if (code == ERRC_SUCCESS) {
      spdlog::info("Login Success! Token: {}", data);
      out_mobile = mobile;
      return data;
    } else {
      spdlog::error("Login Failed. Error Code: {}", code);
    }
  } else {
    spdlog::error("Network error.");
  }
  return "";
}

void cmd_unlock(NetworkClient& client, const std::string& mobile, const std::string& token) {
  tutorial::unlock_request req;
  req.set_mobile(mobile);
  req.set_token(token);

  int dev_no;
  std::cout << "> Enter Bike ID: ";
  std::cin >> dev_no;
  req.set_dev_no(dev_no);

  std::string body;
  req.SerializeToString(&body);
  client.sendRequest(EEVENTID_UNLOCK_REQ, body);

  int code = 0;
  std::string msg;
  if (client.getResponse(code, msg)) {
    if (code == ERRC_SUCCESS) {
      spdlog::info("Bike Unlocked! Enjoy your ride.");
    } else {
      spdlog::warn("Unlock Failed. {} (Code: {})", msg, code);
    }
  } else {
    spdlog::error("Network error.");
  }
}

int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  std::string config_file = "conf/server.ini";
  if (argc >1) {
    config_file = argv[1];
  }

  if (!GlobalConfig::getInstance().load(config_file)) {
    spdlog::error("[Client] Config load, exiting...");
    return -1;
  }

  std::string log_path = GlobalConfig::getInstance().getString("log", "path", "logs/default.log");
  std::string log_level = GlobalConfig::getInstance().getString("log", "level", "info");
  initLogger(log_path, log_level);

  int port = GlobalConfig::getInstance().getInt("server", "port", 8080);
  spdlog::info("[Client] Configuration loaded. Port: {}", port);

  std::string ip = GlobalConfig::getInstance().getString("server", "ip", "127.0.0.1");
  spdlog::info("[Client] Configuration loaded. Ip: {}", ip);

  NetworkClient client(ip, port);
  if (!client.start()) {
    return -1;
  }

  std::string current_mobile = "";//为了用户界面的变换
  std::string current_token = "";

  while (true) {
    std::cout << "\n========================================" << std::endl;
    if (current_mobile.empty()) {
      std::cout << "          FlowBike (Not Logged In)      " << std::endl;
      std::cout << "========================================" << std::endl;
      std::cout << "  1. Get Verify Code (获取验证码)" << std::endl;
      std::cout << "  2. Login (登录)" << std::endl;
      std::cout << "  3. Exit (退出)" << std::endl;
    } else {
      std::cout << "      User: " << current_mobile << "      " << std::endl;
      std::cout << "========================================" << std::endl;
      std::cout << "  1. Unlock Bike (扫码开锁)" << std::endl;
      std::cout << "  2. Logout (注销)" << std::endl;
      std::cout << "  3. Exit (退出)" << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "> Please select: ";

    int choice;
    std::cin >> choice;
    //进行一个输入非数字的错误处理
    if (std::cin.fail()) {
      std::cin.clear();
      std::cin.ignore(10000, '\n');
      spdlog::warn("Invalid input, please enter a number");
      continue;
    }

    if (current_mobile.empty()) {
      if (choice == 1) {
        cmd_get_mobile_code(client);
      } else if (choice == 2) {
        std::string mobile_temp;
        std::string token = cmd_login(client, mobile_temp);

        if (!token.empty()) {
          current_mobile = mobile_temp;
          current_token = token;
        }
      } else if (choice == 3) break;
    } else {
      if (choice == 1) {
        cmd_unlock(client, current_mobile, current_token);
      } else if (choice == 2) {
        current_mobile = "";
        current_token = "";
        spdlog::info("Logged out");
      } else if (choice == 3) break;
    }
  }

  client.Close();
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}