cmake_minimum_required(VERSION 3.19)
project(StatusServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 依赖包 ---

find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(jsoncpp REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_library(HIREDIS_LIB hiredis REQUIRED)

find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

# --- Proto 生成 ---

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto_gen")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

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

set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)

# 静态库在链接可执行文件之前编译，需显式依赖生成的 .pb.h，否则 grpc/mysql 等目标先于 protoc 运行
add_custom_target(status_proto_gen DEPENDS ${PROTO_SRCS} ${PROTO_HDRS})

add_subdirectory(src)
