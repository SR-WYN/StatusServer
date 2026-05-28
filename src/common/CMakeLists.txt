add_library(ss_common STATIC utils.cpp Log.cpp)
target_link_libraries(ss_common PUBLIC statusserver_includes spdlog::spdlog)
