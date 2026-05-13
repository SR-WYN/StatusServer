add_library(ss_redis STATIC RedisMgr.cpp RedisConPool.cpp)
target_link_libraries(ss_redis PUBLIC statusserver_includes ss_config hiredis)
