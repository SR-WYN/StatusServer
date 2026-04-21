cmake_minimum_required(VERSION 3.19)
project(StatusServer VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. 寻找依赖包 ---

find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(jsoncpp REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_library(HIREDIS_LIB hiredis REQUIRED)
find_program(MYSQL_CONFIG_EXECUTABLE NAMES mysql_config)

# 这里的程序名（用于获取 protoc 插件路径）
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

# --- 2. 处理 Proto 文件 (自动化核心) ---

# 假设你的 proto 文件放在项目根目录下的 proto 文件夹中
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

# 定义生成文件的存放目录（构建目录下）
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto_gen")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

# 包含生成后的头文件路径
include_directories(${PROTO_GEN_DIR})

# 自动化逻辑：生成 .pb.cc 和 .grpc.pb.cc
set(PROTO_SRCS "")
set(PROTO_HDRS "")

foreach(proto_file ${PROTO_FILES})
    get_filename_component(proto_name ${proto_file} NAME_WLE)
    
    set(hw_proto_srcs "${PROTO_GEN_DIR}/${proto_name}.pb.cc")
    set(hw_proto_hdrs "${PROTO_GEN_DIR}/${proto_name}.pb.h")
    set(hw_grpc_srcs "${PROTO_GEN_DIR}/${proto_name}.grpc.pb.cc")
    set(hw_grpc_hdrs "${PROTO_GEN_DIR}/${proto_name}.grpc.pb.h")
    
    add_custom_command(
        OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --grpc_out=${PROTO_GEN_DIR}
             --cpp_out=${PROTO_GEN_DIR}
             -I ${PROTO_DIR}
             --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}
             ${proto_file}
        DEPENDS ${proto_file}
        COMMENT "Running gRPC C++ protocol buffer compiler on ${proto_file}"
        VERBATIM
    )
    
    list(APPEND PROTO_SRCS "${hw_proto_srcs}" "${hw_grpc_srcs}")
    list(APPEND PROTO_HDRS "${hw_proto_hdrs}" "${hw_grpc_hdrs}")
endforeach()

# --- 3. 收集其他文件 ---

# 自动收集你手动编写的 include 目录下的头文件
file(GLOB_RECURSE USER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

# 你自己手动添加的业务逻辑源文件
set(SOURCE_FILES
    StatusServer.cpp
    AsioIOServicePool.cpp
    ConfigMgr.cpp
    MySqlDao.cpp
    MysqlMgr.cpp
    MySqlPool.cpp
    RedisConPool.cpp
    RedisMgr.cpp
    utils.cpp
    StatusServiceImpl.cpp
)

# --- 4. 定义生成目标 ---

# 将生成的 PROTO_SRCS 也加入到 executable 中
add_executable(${PROJECT_NAME} 
    ${SOURCE_FILES} 
    ${USER_HEADERS} 
    ${PROTO_SRCS} 
    ${PROTO_HDRS}
)

# --- 5. 配置链接库 ---

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROTO_GEN_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::system
    Boost::thread
    Boost::filesystem
    JsonCpp::JsonCpp
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    hiredis
    mysqlcppconn
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json"
    COMMENT "Copying config.json to execution directory..."
)