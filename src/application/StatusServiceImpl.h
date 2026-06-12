// StatusServiceImpl.h — gRPC 服务实现，处理 GateServer 和 ChatServer 的 RPC 请求
// 通过 NodeRegistry 接口管理节点注册、用户绑定等业务逻辑
#pragma once

#include "NodeRegistry.h"
#include "grpcpp/grpcpp.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/server_context.h>
#include <memory>

using grpc::ServerContext;
using grpc::Status;

using message::BindUserToNodeReq;
using message::BindUserToNodeRsp;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::GetUserChatNodeReq;
using message::GetUserChatNodeRsp;
using message::HeartbeatChatNodeReq;
using message::HeartbeatChatNodeRsp;
using message::RegisterChatNodeReq;
using message::RegisterChatNodeRsp;
using message::StatusService;
using message::UnbindUserReq;
using message::UnbindUserRsp;
using message::UnregisterChatNodeReq;
using message::UnregisterChatNodeRsp;
using message::ValidateTokenReq;
using message::ValidateTokenRsp;

/// gRPC 服务实现 —— 处理 GateServer 和 ChatServer 的 RPC 请求。
/// 不直接依赖任何具体存储实现，通过 NodeRegistry 接口与后端解耦。
class StatusServiceImpl final : public StatusService::Service
{
public:
    /// 构造函数，接收节点注册中心接口实例
    explicit StatusServiceImpl(std::shared_ptr<NodeRegistry> registry);

    // GateServer 调用：获取一个负载最轻的 ChatServer 地址
    Status GetChatServer(ServerContext *context, const GetChatServerReq *request,
                         GetChatServerRsp *reply) override;

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

    // ChatServer 调用：将用户绑定到当前节点
    Status BindUserToNode(ServerContext *context, const BindUserToNodeReq *request,
                          BindUserToNodeRsp *reply) override;

    // GateServer 调用：解绑用户与节点的绑定关系
    Status UnbindUser(ServerContext *context, const UnbindUserReq *request,
                      UnbindUserRsp *reply) override;

    // FileServer 调用：验证 Token 有效性
    Status ValidateToken(ServerContext *context, const ValidateTokenReq *request,
                         ValidateTokenRsp *reply) override;

private:
    // 将 token 存入 Redis，供后续登录验证
    void insertToken(int uid, const std::string &token);

    // 节点注册中心接口（通过依赖注入传入）
    std::shared_ptr<NodeRegistry> _registry;
};
