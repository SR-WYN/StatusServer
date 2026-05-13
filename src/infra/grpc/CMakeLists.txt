add_library(ss_grpc STATIC ChatGrpcClient.cpp ChatConPool.cpp)
add_dependencies(ss_grpc status_proto_gen)
target_link_libraries(ss_grpc PUBLIC statusserver_includes gRPC::grpc++ protobuf::libprotobuf ss_config ss_common)
