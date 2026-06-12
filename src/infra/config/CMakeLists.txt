add_library(ss_config STATIC ConfigMgr.cpp)
target_link_libraries(ss_config PUBLIC ss_includes Boost::filesystem JsonCpp::JsonCpp spdlog::spdlog)
