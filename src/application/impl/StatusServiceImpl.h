// StatusServiceImpl.h — gRPC 服务实现，处理 GateServer 和 ChatServer 的 RPC 请求
// 通过 NodeRegistry 接口管理节点注册、用户绑定等业务逻辑
#pragma once

#include "FileTokenRepository.h"
#include "GateNotifyClient.h"
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
using message::DeleteFileTokenReq;
using message::DeleteFileTokenRsp;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::GetFileServerReq;
using message::GetFileServerRsp;
using message::GetUserNodeReq;
using message::GetUserNodeRsp;
using message::HeartbeatNodeReq;
using message::HeartbeatNodeRsp;
using message::RegisterNodeReq;
using message::RegisterNodeRsp;
using message::StatusService;
using message::UnbindUserReq;
using message::UnbindUserRsp;
using message::UnregisterNodeReq;
using message::UnregisterNodeRsp;
using message::ValidateTokenReq;
using message::ValidateTokenRsp;

/// gRPC 服务实现 —— 处理 GateServer 和 ChatServer 的 RPC 请求。
/// 不直接依赖任何具体存储实现，通过 NodeRegistry 接口与后端解耦。
class StatusServiceImpl final : public StatusService::Service
{
public:
    /// 构造函数，接收节点注册中心接口实例、文件 token 仓库和 GateServer 通知客户端
    /// @param registry 节点注册中心
    /// @param file_token_repo 文件传输临时 token 仓库
    /// @param gate_client GateServer 通知客户端（可为 nullptr）
    StatusServiceImpl(std::shared_ptr<NodeRegistry> registry,
                      std::shared_ptr<FileTokenRepository> file_token_repo,
                      std::shared_ptr<GateNotifyClient> gate_client);

    // GateServer 调用：获取一个负载最轻的 ChatServer 地址
    Status GetChatServer(ServerContext *context, const GetChatServerReq *request,
                         GetChatServerRsp *reply) override;

    // ChatServer 调用：注册节点到注册中心
    Status RegisterNode(ServerContext *context, const RegisterNodeReq *request,
                        RegisterNodeRsp *reply) override;

    // ChatServer 调用：从注册中心注销节点
    Status UnregisterNode(ServerContext *context, const UnregisterNodeReq *request,
                          UnregisterNodeRsp *reply) override;

    // ChatServer 调用：节点心跳续期
    Status HeartbeatNode(ServerContext *context, const HeartbeatNodeReq *request,
                         HeartbeatNodeRsp *reply) override;

    // GateServer 调用：查询用户当前绑定的节点
    Status GetUserNode(ServerContext *context, const GetUserNodeReq *request,
                       GetUserNodeRsp *reply) override;

    // ChatServer 调用：将用户绑定到当前节点
    Status BindUserToNode(ServerContext *context, const BindUserToNodeReq *request,
                          BindUserToNodeRsp *reply) override;

    // GateServer 调用：解绑用户与节点的绑定关系
    Status UnbindUser(ServerContext *context, const UnbindUserReq *request,
                      UnbindUserRsp *reply) override;

    // FileServer 调用：验证 Token 有效性
    Status ValidateToken(ServerContext *context, const ValidateTokenReq *request,
                         ValidateTokenRsp *reply) override;

    // ChatServer 调用：获取一个可用的 FileServer 地址及临时 token
    Status GetFileServer(ServerContext *context, const GetFileServerReq *request,
                         GetFileServerRsp *reply) override;

    // ChatServer 调用：删除指定用户的文件传输临时 token
    Status DeleteFileToken(ServerContext *context, const DeleteFileTokenReq *request,
                           DeleteFileTokenRsp *reply) override;

private:
    // 将 token 存入 Redis，供后续登录验证
    void insertToken(int uid, const std::string &token);

    // 节点注册中心接口（通过依赖注入传入）
    std::shared_ptr<NodeRegistry> _registry;

    // 文件传输临时 token 仓库
    std::shared_ptr<FileTokenRepository> _file_token_repo;

    // GateServer 通知客户端
    std::shared_ptr<GateNotifyClient> _gate_client;
};
