add_library(ss_net STATIC AsioIOServicePool.cpp)
target_link_libraries(ss_net PUBLIC statusserver_includes Boost::system Boost::thread)
