add_library(ss_application STATIC StatusServiceImpl.cpp NodeRegistry.cpp)
add_dependencies(ss_application status_proto_gen)
target_link_libraries(ss_application PUBLIC
    statusserver_includes
    ss_domain
    ss_common
    ss_config
    ss_redis
)
