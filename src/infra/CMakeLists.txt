add_subdirectory(base)
add_subdirectory(log)
add_subdirectory(config)
add_subdirectory(redis)

add_library(ss_infra INTERFACE)

target_link_libraries(ss_infra INTERFACE
    ss_includes
    common
    ss_domain
    ss_base
    ss_log
    ss_config
    ss_redis
    Boost::system
    Boost::thread
    Boost::filesystem
    JsonCpp::JsonCpp
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    redis++::redis++
    spdlog::spdlog
)
