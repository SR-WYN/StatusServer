#pragma once

#include "grpcpp/grpcpp.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/server_context.h>
#include <mutex>
#include <vector>

using grpc::ServerContext;
using grpc::Status;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;
using message::LoginReq;
using message::LoginRsp;

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
    Status Login(ServerContext* context,const LoginReq* request,LoginRsp* reply) override;
private:
    void insertToken(int uid,const std::string& token);
    ChatServer getChatServer();
    std::vector<ChatServer> _servers;
    std::unordered_map<int, std::string> _tokens;
    std::vector<int> _connection_counts;
    std::mutex _server_mutex;
    std::mutex _token_mutex;
};