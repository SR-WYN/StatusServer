add_library(ss_redis STATIC RedisMgr.cpp)

target_link_libraries(ss_redis PUBLIC
    ss_includes
    ss_base
    ss_config
    ss_log
    redis++::redis++
    spdlog::spdlog
)
