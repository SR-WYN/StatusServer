#pragma once

#include "grpcpp/grpcpp.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/server_context.h>
#include "ChatNodeRegistry.h"

using grpc::ServerContext;
using grpc::Status;

using message::BindUserToNodeReq;
using message::BindUserToNodeRsp;
using message::GetChatNodeReq;
using message::GetChatNodeRsp;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::GetUserChatNodeReq;
using message::GetUserChatNodeRsp;
using message::HeartbeatChatNodeReq;
using message::HeartbeatChatNodeRsp;
using message::LoginReq;
using message::LoginRsp;
using message::RegisterChatNodeReq;
using message::RegisterChatNodeRsp;
using message::StatusService;
using message::UnregisterChatNodeReq;
using message::UnregisterChatNodeRsp;
using message::UnbindUserReq;
using message::UnbindUserRsp;

class StatusServiceImpl final : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext *context, const GetChatServerReq *request,
                         GetChatServerRsp *reply) override;
    Status Login(ServerContext *context, const LoginReq *request, LoginRsp *reply) override;
    Status RegisterChatNode(ServerContext *context, const RegisterChatNodeReq *request,
                            RegisterChatNodeRsp *reply) override;
    Status UnregisterChatNode(ServerContext *context, const UnregisterChatNodeReq *request,
                              UnregisterChatNodeRsp *reply) override;
    Status HeartbeatChatNode(ServerContext *context, const HeartbeatChatNodeReq *request,
                             HeartbeatChatNodeRsp *reply) override;
    Status GetUserChatNode(ServerContext *context, const GetUserChatNodeReq *request,
                           GetUserChatNodeRsp *reply) override;
    Status GetChatNode(ServerContext *context, const GetChatNodeReq *request,
                       GetChatNodeRsp *reply) override;
    Status BindUserToNode(ServerContext *context, const BindUserToNodeReq *request,
                          BindUserToNodeRsp *reply) override;
    Status UnbindUser(ServerContext *context, const UnbindUserReq *request,
                      UnbindUserRsp *reply) override;

private:
    void insertToken(int uid, const std::string &token);
    static void fillNodeReply(const RegisteredChatNode &node, GetChatNodeRsp *reply);
};
