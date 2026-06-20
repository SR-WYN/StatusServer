add_library(ss_base STATIC
    Singleton.h
    ThreadPool.h
    ThreadPool.cpp
    ThreadPoolMgr.h
    ThreadPoolMgr.cpp
    utils.cpp
)

target_link_libraries(ss_base PUBLIC ss_includes)
