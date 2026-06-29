add_library(ss_application STATIC
    interface/NodeRegistry.h
    interface/GateNotifyClient.h
    interface/ChatNotifyClient.h
    interface/FileTokenRepository.h
    impl/StatusServiceImpl.cpp
    impl/RedisNodeRegistryImpl.cpp
    impl/RedisFileTokenRepositoryImpl.cpp
    impl/GateNotifyClientImpl.cpp
    impl/ChatNotifyClientImpl.cpp
)

add_dependencies(ss_application ss_proto_gen)

target_link_libraries(ss_application PUBLIC
    ss_includes
    ss_domain
    ss_infra
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
)
