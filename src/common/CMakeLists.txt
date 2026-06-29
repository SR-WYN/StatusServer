add_library(common STATIC
    utils/utils.cpp
)

target_link_libraries(common PUBLIC
    ss_includes
    Boost::boost
    spdlog::spdlog
)