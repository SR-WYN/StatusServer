# 扁平 #include：各子目录均加入 INTERFACE，源文件写 "Foo.h" 即可

# 父目录生成的 proto 尚不存在；在本目录声明 GENERATED，便于 add_executable 通过校验（CMake 3.19）
set_source_files_properties(${PROTO_SRCS} PROPERTIES GENERATED TRUE)

add_library(statusserver_includes INTERFACE)
target_include_directories(statusserver_includes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/net
    ${CMAKE_CURRENT_SOURCE_DIR}/application
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/config
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/redis
    ${CMAKE_CURRENT_SOURCE_DIR}/infra/grpc
    ${PROTO_GEN_DIR}
)

add_subdirectory(common)
add_subdirectory(domain)
add_subdirectory(infra/config)
add_subdirectory(infra/redis)
add_subdirectory(infra/grpc)
add_subdirectory(net)
add_subdirectory(application)
add_subdirectory(app)

add_executable(${PROJECT_NAME}
    app/StatusServer.cpp
    ${PROTO_SRCS}
)

# 可执行文件放在顶层 build/，便于运行 ./build/StatusServer（add_executable 在 src/ 时默认会生成 build/src/）
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROTO_GEN_DIR})

# ss_net 在 ss_application 之前，与 Gate 静态链接顺序一致
target_link_libraries(${PROJECT_NAME} PRIVATE
    statusserver_includes
    statusserver_app_dir
    ss_net
    ss_application
    ss_common
    ss_config
    ss_redis
    ss_grpc
    Boost::system
    Boost::thread
    Boost::filesystem
    JsonCpp::JsonCpp
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    hiredis
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json"
    COMMENT "Copying config.json to execution directory..."
)
