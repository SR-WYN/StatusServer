add_library(ss_base STATIC
    Singleton.h
    utils.cpp
)

target_link_libraries(ss_base PUBLIC ss_includes)
