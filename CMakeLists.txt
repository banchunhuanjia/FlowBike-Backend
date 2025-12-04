cmake_minimum_required(VERSION 3.10)
project(SharedBike C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/protos/bike.proto)
set(GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/bike.pb.cc)
set(GENERATED_HDR ${CMAKE_CURRENT_BINARY_DIR}/bike.pb.h)

pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(MYSQL REQUIRED mysqlclient)
pkg_check_modules(HIREDIS REQUIRED hiredis)

# proto 的生成命令
add_custom_command(
  OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
          --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
          --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/protos
          --experimental_allow_proto3_optional
          ${PROTO_FILE}
  DEPENDS ${PROTO_FILE}
  COMMENT "Running protoc with experimental proto3 optional support"
  VERBATIM
)

# 创建一个虚拟目标，专门用于代表“生成 Protobuf 文件”这个动作
add_custom_target(generate_proto ALL
  DEPENDS ${GENERATED_SRC} ${GENERATED_HDR}
)


# ==========================================
# 1. 基础库层 (Protobuf & Utils)
# ==========================================

# 将protobuf独立成库，防止重复编译
add_library(proto_lib STATIC
  ${GENERATED_SRC}
  ${GENERATED_HDR} #把 .h 也加入源码列表，帮助 IDE 识别
)
add_dependencies(proto_lib generate_proto)
target_link_libraries(proto_lib PUBLIC
  ${Protobuf_LIBRARIES}
)
target_include_directories(proto_lib PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Protobuf_INCLUDE_DIRS}
)

# iniparser库
add_library(config_lib STATIC
  "extern/iniparser/iniparser.c"
  "extern/iniparser/dictionary.c"
  "src/global_config.cpp"
)
target_include_directories(config_lib
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/extern/iniparser"
) # 隐藏一下底层库

# 线程池库
add_library(threadpool_lib STATIC
  "src/threadpool.cpp"
)
target_include_directories(threadpool_lib PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(threadpool_lib PUBLIC
  Threads::Threads
)

# ==========================================
# 2. 数据模型层 (Events)
# ==========================================

# 事件库
add_library(ievent_lib STATIC
  "src/ievent.cpp"
  "src/mobilecodereqev.cpp"
  "src/mobilecoderspev.cpp"
  "src/loginreq.cpp"
  "src/loginrsp.cpp"
  "src/errormanager.cpp"
  "src/unlockreq.cpp"
  "src/unlockrsp.cpp"
)
target_include_directories(ievent_lib PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(ievent_lib PUBLIC
  proto_lib
)
# [关键修复] 显式强制依赖，防止 ievent.cpp 抢跑
add_dependencies(ievent_lib generate_proto)

# ==========================================
# 3. 核心业务层 (Server Core)
# ==========================================

# 为了解决循环依赖所以把他们打包成一个库
add_library(server_core_lib STATIC
  "src/dispatchmsgservice.cpp"
  "src/networkinterface.cpp"
  "src/tcpconnection.cpp"
  "src/usereventhandler.cpp"
  "src/mysqlconn.cpp"
  "src/connectionpool.cpp"
  "src/userdao.cpp"
  "src/bikedao.cpp"
  "src/bikeeventhandler.cpp"
  "src/redistool.cpp"
)
target_include_directories(server_core_lib PUBLIC
  "${PROJECT_SOURCE}/include"
  ${LIBEVENT_INCLUDE_DIRS}
  ${MYSQL_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIRS}
)
target_link_libraries(server_core_lib PUBLIC
  ievent_lib
  threadpool_lib
  config_lib
  ${LIBEVENT_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${HIREDIS_LIBRARIES}
)
# [关键修复] 显式强制依赖，因为 server_core 代码也包含 bike.pb.h
add_dependencies(server_core_lib generate_proto)

# ==========================================
# 4. 可执行文件层
# ==========================================

# 处理如何寻找ini文件
add_custom_target(copy_conf
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/conf"
    "${CMAKE_CURRENT_BINARY_DIR}/conf"
  COMMENT "Copying config files to build directory..."
  VERBATIM
)


add_executable(flowbike_server
  "src/main.cpp"
)
target_include_directories(flowbike_server PRIVATE 
  "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(flowbike_server PRIVATE
  server_core_lib
  spdlog::spdlog
)
add_dependencies(flowbike_server copy_conf)

add_executable(flowbike_client
  "src/simple_client.cpp"
  "src/network_client.cpp"
  ${GENERATED_SRC}
)
target_include_directories(flowbike_client PRIVATE
  "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(flowbike_client PRIVATE
  proto_lib
  config_lib
  Threads::Threads
  spdlog::spdlog
)
# [关键修复] 显式强制依赖
add_dependencies(flowbike_client generate_proto)
add_dependencies(flowbike_client copy_conf)



add_executable(benchmark
    "src/benchmark.cpp"
    "src/network_client.cpp" # 复用网络客户端代码
    ${GENERATED_SRC}
)

# 包含路径
target_include_directories(benchmark PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

# 链接库
target_link_libraries(benchmark PRIVATE
    proto_lib
    spdlog::spdlog
    Threads::Threads
)
# 依赖
add_dependencies(benchmark generate_proto)


