add_library(ss_redis STATIC RedisMgr.cpp)
target_link_libraries(ss_redis PUBLIC statusserver_includes ss_config redis++)
