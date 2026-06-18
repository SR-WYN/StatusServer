add_library(ss_log STATIC Log.cpp)

target_link_libraries(ss_log PUBLIC ss_includes spdlog::spdlog)
