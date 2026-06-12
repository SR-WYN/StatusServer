add_library(ss_common STATIC utils.cpp Log.cpp)
target_link_libraries(ss_common PUBLIC ss_includes spdlog::spdlog)
