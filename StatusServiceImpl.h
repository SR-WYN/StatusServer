#pragma once

#include "grpcpp/grpcpp.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <mutex>
#include <vector>

using grpc::ServerContext;
using grpc::Status;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

struct ChatServer
{
    std::string host;
    std::string port;
};

class StatusServiceImpl final : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
                         GetChatServerRsp* reply) override;

    std::vector<ChatServer> _servers;
    /// Per-server count used for least-connections pick (incremented on each successful dispatch).
    std::vector<int> _connection_counts;
    std::mutex _server_mutex;
};