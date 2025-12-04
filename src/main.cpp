#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>
#include <cstring>
#include <unistd.h>

#include "threadpool.h"
#include "dispatchmsgservice.h"
#include "usereventhandler.h"
#include "bikeeventhandler.h"
#include "networkinterface.h"
#include "mysqlconn.h"
#include "connectionpool.h"

#include "global_config.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/fmt/ostr.h"// 告诉 spdlog，如果遇到不认识的类型，试着用 C++ 标准的 ostream (<<) 来输出


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


int main(int argc, char** argv) {
  std::string config_file = "conf/server.ini";
  if (argc > 1) {
    config_file = argv[1];
  }

  if (!GlobalConfig::getInstance().load(config_file)) {
    std::cerr << "[Main] Config load failed, exiting..." << std::endl;
    return -1;
  }

  std::string log_path = GlobalConfig::getInstance().getString("log", "path", "logs/default.log");
  std::string log_level = GlobalConfig::getInstance().getString("log", "level", "info");

  initLogger(log_path, log_level);

  spdlog::info("Logger initialized from config");

  spdlog::info("--- Initializing MySQL Pool ---");
  std::string db_ip = GlobalConfig::getInstance().getString("mysql", "ip", "127.0.0.1");
  int db_port = GlobalConfig::getInstance().getInt("mysql", "port", 8080);
  std::string db_user = GlobalConfig::getInstance().getString("mysql", "user", "root");
  std::string db_pwd = GlobalConfig::getInstance().getString("mysql", "pwd", "byjlz20050725");
  std::string db_name = GlobalConfig::getInstance().getString("mysql", "db", "flowbike");
  int min_conn = GlobalConfig::getInstance().getInt("mysql", "min_conn", 5);
  int max_conn = GlobalConfig::getInstance().getInt("mysql", "max_conn", 10);
  int maxidletime = GlobalConfig::getInstance().getInt("mysql", "max_conn", 300000);
  ConnectionPool::getConnectionPool()->init(db_user, db_pwd, db_name, db_ip, db_port, min_conn, max_conn, maxidletime);

  //做一个快速的健康检查
  {
    auto conn = ConnectionPool::getConnectionPool()->getConnection();
    if (!conn) {
      spdlog::critical("[Main] Database unreached, System verify failed");
      return -1;
    }
    spdlog::info("[Main] Database connection chack PASSED");
  }

  spdlog::info("--- FlowBike Server Main ---");
  spdlog::info("[Main Thread ID] {}", std::this_thread::get_id());

  int port = GlobalConfig::getInstance().getInt("server", "port", 8080);
  spdlog::info("[Main] Configuration loaded. Port: {}", port);

  int thread_num = GlobalConfig::getInstance().getInt("server", "thread_num", 8);
  spdlog::info("[Main] Configuration loaded. Thread_num: {}", thread_num);

  try {
    ThreadPool thread_pool(thread_num);
    DispatchMsgService dispatch_service(thread_pool);
    UserEventHandler user_handler(&dispatch_service);
    
    //忘记在服务端进行激活
    BikeEventHandler bike_handler(&dispatch_service);

    auto network_interface = std::make_unique<NetworkInterface>(&dispatch_service);
    dispatch_service.set_networkinterface(network_interface.get());
    
    
    std::thread network_thread([&](){
      network_interface->start(port);
    });

    spdlog::info("[Main] Server is running on port {}. Press Ctrl+C to stop",port);

    if (network_thread.joinable()) {
      network_thread.join();
    }
  } catch (const std::exception& e) {
    spdlog::error("[Main] An exception occured: {}", e.what());
    return 1;
  }

  spdlog::info("[Main] Server has been shut down successfully");
  
  // 强制刷新硬盘
  spdlog::shutdown();
  return 0;
}