add_executable(statusserver_tests
    test_utils.cpp
    test_node_registry.cpp
    MemoryNodeRegistryImpl.h
)

target_include_directories(statusserver_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common/utils
    ${CMAKE_SOURCE_DIR}/src/common/constants
    ${CMAKE_SOURCE_DIR}/src/domain
    ${CMAKE_SOURCE_DIR}/src/application/interface
    ${CMAKE_SOURCE_DIR}/src/application/impl
    ${CMAKE_SOURCE_DIR}/src/infra/base
    ${CMAKE_SOURCE_DIR}/src/infra/log
    ${CMAKE_SOURCE_DIR}/src/infra/config
    ${CMAKE_SOURCE_DIR}/src/infra/redis
    ${PROTO_GEN_DIR}
)

target_link_libraries(statusserver_tests PRIVATE
    ss_application
    ss_infra
    common
    GTest::gtest_main
)

add_test(NAME UrlEncodeTest COMMAND statusserver_tests --gtest_filter=UrlEncodeTest*)
add_test(NAME UrlDecodeTest COMMAND statusserver_tests --gtest_filter=UrlDecodeTest*)
add_test(NAME ParseLevelTest COMMAND statusserver_tests --gtest_filter=ParseLevelTest*)
add_test(NAME DeferTest COMMAND statusserver_tests --gtest_filter=DeferTest*)
add_test(NAME NodeRegistryTest COMMAND statusserver_tests --gtest_filter=NodeRegistryTest*)
