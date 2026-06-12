add_library(ss_application STATIC StatusServiceImpl.cpp RedisNodeRegistryImpl.cpp)
add_dependencies(ss_application ss_proto_gen)
target_link_libraries(ss_application PUBLIC
    ss_includes
    ss_domain
    ss_common
    ss_config
    ss_redis
)
