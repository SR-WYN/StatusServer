#pragma once

#include "grpcpp/grpcpp.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/server_context.h>
#include "NodeRegistry.h"

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

/// gRPC 服务实现 —— 处理 GateServer 和 ChatServer 的 RPC 请求
class StatusServiceImpl final : public StatusService::Service
{
public:
    StatusServiceImpl();

    // GateServer 调用：获取一个负载最轻的 ChatServer 地址
    Status GetChatServer(ServerContext *context, const GetChatServerReq *request,
                         GetChatServerRsp *reply) override;

    // GateServer 调用：验证用户登录 token
    Status Login(ServerContext *context, const LoginReq *request, LoginRsp *reply) override;

    // ChatServer 调用：注册节点到注册中心
    Status RegisterChatNode(ServerContext *context, const RegisterChatNodeReq *request,
                            RegisterChatNodeRsp *reply) override;

    // ChatServer 调用：从注册中心注销节点
    Status UnregisterChatNode(ServerContext *context, const UnregisterChatNodeReq *request,
                              UnregisterChatNodeRsp *reply) override;

    // ChatServer 调用：节点心跳续期
    Status HeartbeatChatNode(ServerContext *context, const HeartbeatChatNodeReq *request,
                             HeartbeatChatNodeRsp *reply) override;

    // GateServer 调用：查询用户当前绑定的节点
    Status GetUserChatNode(ServerContext *context, const GetUserChatNodeReq *request,
                           GetUserChatNodeRsp *reply) override;

    // GateServer 调用：按名称查询节点信息
    Status GetChatNode(ServerContext *context, const GetChatNodeReq *request,
                       GetChatNodeRsp *reply) override;

    // ChatServer 调用：将用户绑定到当前节点
    Status BindUserToNode(ServerContext *context, const BindUserToNodeReq *request,
                          BindUserToNodeRsp *reply) override;

    // GateServer 调用：解绑用户与节点的绑定关系
    Status UnbindUser(ServerContext *context, const UnbindUserReq *request,
                      UnbindUserRsp *reply) override;

private:
    // 将 token 存入 Redis，供后续 Login 验证
    void insertToken(int uid, const std::string &token);
    // 将 NodeInfo 填充到 GetChatNodeRsp 响应中
    static void fillNodeReply(const NodeInfo &node, GetChatNodeRsp *reply);
};
