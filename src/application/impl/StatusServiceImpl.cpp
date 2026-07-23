// StatusServiceImpl.cpp — gRPC 服务实现
// 通过 NodeRegistry 接口转发所有节点注册/发现/绑定的业务逻辑
#include "StatusServiceImpl.h"

#include "Log.h"
#include "ThreadPoolMgr.h"
#include "const.h"
#include "utils.h"

#include <chrono>

StatusServiceImpl::StatusServiceImpl(std::shared_ptr<NodeRegistry> registry,
                                     std::shared_ptr<FileTokenRepository> file_token_repo,
                                     std::shared_ptr<GateNotifyClient> gate_client,
                                     std::shared_ptr<ChatNotifyClient> chat_client)
    : _registry(std::move(registry)),
      _file_token_repo(std::move(file_token_repo)),
      _gate_client(std::move(gate_client)),
      _chat_client(std::move(chat_client))
{
    Log::info(LogModule::Grpc, "StatusServiceImpl constructed, cleaning up expired nodes");
    _registry->cleanupExpiredNodes();
}

// 异步通知 GateServer 清理用户 session
void StatusServiceImpl::notifyGateUserOffline(int uid)
{
    if (!_gate_client)
    {
        Log::debug(LogModule::Grpc, "notifyGateUserOffline: no gate client configured");
        return;
    }

    int uidCopy = uid;
    auto gateClient = _gate_client;
    ThreadPoolMgr::getInstance().enqueueGrpcClient([gateClient, uidCopy]() {
        if (!gateClient->notifyUserOffline(uidCopy))
        {
            Log::warn(LogModule::Grpc, "notifyGateUserOffline: failed for uid={}", uidCopy);
        }
        else
        {
            Log::info(LogModule::Grpc, "notifyGateUserOffline: success for uid={}", uidCopy);
        }
    });
}

// 异步通知 GateServer 刷新用户 session TTL
void StatusServiceImpl::notifyGateUserOnline(int uid)
{
    if (!_gate_client)
    {
        Log::debug(LogModule::Grpc, "notifyGateUserOnline: no gate client configured");
        return;
    }

    int uidCopy = uid;
    auto gateClient = _gate_client;
    ThreadPoolMgr::getInstance().enqueueGrpcClient([gateClient, uidCopy]() {
        if (!gateClient->notifyUserOnline(uidCopy))
        {
            Log::warn(LogModule::Grpc, "notifyGateUserOnline: failed for uid={}", uidCopy);
        }
        else
        {
            Log::info(LogModule::Grpc, "notifyGateUserOnline: success for uid={}", uidCopy);
        }
    });
}

Status StatusServiceImpl::GetChatServer(ServerContext *context, const GetChatServerReq *request,
                                        GetChatServerRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    int uid = request->uid();
    Log::info(LogModule::Grpc, "GetChatServer: uid={}", uid);

    if (uid <= 0)
    {
        Log::warn(LogModule::Grpc, "GetChatServer: invalid uid={}", uid);
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    // 1. 查询旧节点，先尝试通知旧节点踢掉用户（失败不阻塞登录）
    auto old_node = _registry->getNodeForUser(uid);
    if (old_node && _chat_client)
    {
        Log::info(LogModule::Grpc, "GetChatServer: kicking old session uid={} node={}",
                  uid, old_node->name);
        if (!_chat_client->notifyKickUser(old_node->rpc_host, old_node->rpc_port, uid,
                                          "new login"))
        {
            Log::warn(LogModule::Grpc, "GetChatServer: kick old session failed uid={} node={}",
                      uid, old_node->name);
        }
    }

    // 2. 统一清理旧登录数据（解绑节点 + 删 token）
    if (!_registry->clearUserLoginData(uid))
    {
        Log::warn(LogModule::Grpc, "GetChatServer: clearUserLoginData failed uid={}", uid);
    }

    // 3. 通知 GateServer 清理旧 session
    notifyGateUserOffline(uid);

    // 4. 负载均衡选择新节点
    auto server = _registry->selectLeastLoadedNode();
    if (!server)
    {
        Log::warn(LogModule::Grpc, "GetChatServer: no available chat server for uid={}", uid);
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    // 5. 生成新 token 并保存（短 TTL）
    reply->set_host(server->client_host);
    reply->set_port(server->client_port);
    reply->set_token(utils::uuid::generate());

    if (!_registry->saveToken(uid, reply->token()))
    {
        Log::error(LogModule::Grpc, "GetChatServer: failed to save token for uid={}", uid);
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc,
              "GetChatServer: assigned uid={} to {}:{} token={} cost={}ms",
              uid, server->client_host, server->client_port,
              reply->token(), cost_ms);
    return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, const std::string &token)
{
    if (!_registry->saveToken(uid, token))
    {
        Log::warn(LogModule::Grpc, "insertToken: failed to save token for uid={}", uid);
        return;
    }
    Log::debug(LogModule::Grpc, "insertToken: uid={} token={}", uid, token);
}

Status StatusServiceImpl::RegisterNode(ServerContext *context,
                                           const RegisterNodeReq *request,
                                           RegisterNodeRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    Log::info(LogModule::Grpc, "RegisterNode: name={} instance={} client={}:{} rpc={}:{}",
              request->name(), request->instance_id(), request->client_host(),
              request->client_port(), request->rpc_host(), request->rpc_port());

    NodeInfo node;
    node.name = request->name();
    node.client_host = request->client_host();
    node.client_port = request->client_port();
    node.rpc_host = request->rpc_host();
    node.rpc_port = request->rpc_port();
    node.instance_id = request->instance_id();

    if (!_registry->registerNode(node))
    {
        Log::warn(LogModule::Grpc, "RegisterNode: rejected or failed for node {}",
                  request->name());
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_name(node.name);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc, "RegisterNode: node {} instance {} registered cost={}ms",
              request->name(), request->instance_id(), cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::UnregisterNode(ServerContext *context,
                                             const UnregisterNodeReq *request,
                                             UnregisterNodeRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    Log::info(LogModule::Grpc, "UnregisterNode: name={} instance={}", request->name(),
              request->instance_id());

    if (!_registry->unregisterNode(request->name(), request->instance_id()))
    {
        Log::warn(LogModule::Grpc, "UnregisterNode: failed for node {} instance {}",
                  request->name(), request->instance_id());
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc, "UnregisterNode: node {} instance {} unregistered cost={}ms",
              request->name(), request->instance_id(), cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::HeartbeatNode(ServerContext *context,
                                            const HeartbeatNodeReq *request,
                                            HeartbeatNodeRsp *reply)
{
    (void)context;
    Log::debug(LogModule::Grpc, "HeartbeatNode: name={} instance={}", request->name(),
               request->instance_id());

    if (!_registry->heartbeat(request->name(), request->instance_id()))
    {
        Log::warn(LogModule::Grpc, "HeartbeatNode: failed for node {} instance {}",
                  request->name(), request->instance_id());
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    return Status::OK;
}

Status StatusServiceImpl::GetUserNode(ServerContext *context, const GetUserNodeReq *request,
                                          GetUserNodeRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    Log::info(LogModule::Grpc, "GetUserNode: uid={}", request->uid());

    if (request->uid() <= 0)
    {
        Log::warn(LogModule::Grpc, "GetUserNode: invalid uid={}", request->uid());
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    auto node = _registry->getNodeForUser(request->uid());
    if (!node)
    {
        Log::debug(LogModule::Grpc, "GetUserNode: no node found for uid={}", request->uid());
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_node_name(node->name);
    reply->set_rpc_host(node->rpc_host);
    reply->set_rpc_port(node->rpc_port);
    reply->set_client_host(node->client_host);
    reply->set_client_port(node->client_port);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc, "GetUserNode: uid={} -> node {} ({}:{}) cost={}ms",
              request->uid(), node->name, node->client_host, node->client_port, cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::BindUserToNode(ServerContext *context, const BindUserToNodeReq *request,
                                         BindUserToNodeRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    Log::info(LogModule::Grpc, "BindUserToNode: uid={} node={}", request->uid(),
              request->node_name());

    if (request->uid() <= 0)
    {
        Log::warn(LogModule::Grpc, "BindUserToNode: invalid uid={}", request->uid());
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    if (!_registry->bindUser(request->uid(), request->node_name()))
    {
        Log::warn(LogModule::Grpc, "BindUserToNode: failed to bind uid={} to node {}",
                   request->uid(), request->node_name());
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc, "BindUserToNode: uid={} bound to node {} cost={}ms",
              request->uid(), request->node_name(), cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::UnbindUser(ServerContext *context, const UnbindUserReq *request,
                                     UnbindUserRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    int uid = request->uid();
    Log::info(LogModule::Grpc, "UnbindUser: uid={}", uid);

    if (uid <= 0)
    {
        Log::warn(LogModule::Grpc, "UnbindUser: invalid uid={}", uid);
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    if (!_registry->clearUserLoginData(uid))
    {
        Log::warn(LogModule::Grpc, "UnbindUser: failed to clear login data uid={}", uid);
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    // 通过 gRPC 通知 GateServer 用户下线（投递到 gRPC 客户端池，不阻塞当前线程）
    notifyGateUserOffline(uid);

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc, "UnbindUser: uid={} unbound cost={}ms", uid, cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::Logout(ServerContext *context, const LogoutReq *request,
                                 LogoutRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    int uid = request->uid();
    Log::info(LogModule::Grpc, "Logout: uid={}", uid);

    if (uid <= 0)
    {
        Log::warn(LogModule::Grpc, "Logout: invalid uid={}", uid);
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    // 统一清理登录数据并通知 GateServer
    if (!_registry->clearUserLoginData(uid))
    {
        Log::warn(LogModule::Grpc, "Logout: clearUserLoginData failed uid={}", uid);
    }
    notifyGateUserOffline(uid);

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc, "Logout: uid={} cost={}ms", uid, cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::RefreshTokenTTL(ServerContext *context,
                                          const RefreshTokenTTLReq *request,
                                          RefreshTokenTTLRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    int uid = request->uid();
    Log::debug(LogModule::Grpc, "RefreshTokenTTL: uid={}", uid);

    if (uid <= 0)
    {
        Log::warn(LogModule::Grpc, "RefreshTokenTTL: invalid uid={}", uid);
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    if (!_registry->refreshTokenTTL(uid))
    {
        Log::warn(LogModule::Grpc, "RefreshTokenTTL: failed for uid={}", uid);
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::debug(LogModule::Grpc, "RefreshTokenTTL: uid={} cost={}ms", uid, cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::NotifyUserOnline(ServerContext *context,
                                           const UserOnlineReq *request,
                                           UserOnlineRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    int uid = request->uid();
    Log::info(LogModule::Grpc, "NotifyUserOnline: uid={}", uid);

    if (uid <= 0)
    {
        Log::warn(LogModule::Grpc, "NotifyUserOnline: invalid uid={}", uid);
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    // 异步通知 GateServer 刷新 session TTL（失败不阻塞回复）
    notifyGateUserOnline(uid);

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc, "NotifyUserOnline: uid={} cost={}ms", uid, cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::ValidateToken(ServerContext *context, const ValidateTokenReq *request,
                                        ValidateTokenRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    Log::debug(LogModule::Grpc, "ValidateToken: uid={} token={}", request->uid(),
               request->token());

    if (request->uid() <= 0)
    {
        Log::warn(LogModule::Grpc, "ValidateToken: invalid uid={}", request->uid());
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    int err = _registry->validateToken(request->uid(), request->token());
    if (err != ErrorCodes::SUCCESS)
    {
        Log::warn(LogModule::Grpc, "ValidateToken: failed uid={} err={}", request->uid(), err);
        reply->set_error(err);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_uid(request->uid());

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::debug(LogModule::Grpc, "ValidateToken: success uid={} cost={}ms", request->uid(),
               cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::ResolveToken(ServerContext *context, const ResolveTokenReq *request,
                                     ResolveTokenRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    const std::string &token = request->token();
    Log::info(LogModule::Grpc, "ResolveToken: token_len={}", token.length());

    if (token.empty())
    {
        Log::warn(LogModule::Grpc, "ResolveToken: empty token");
        reply->set_error(ErrorCodes::TOKEN_INVALID);
        return Status::OK;
    }

    int uid = _registry->resolveToken(token);
    if (uid <= 0)
    {
        Log::warn(LogModule::Grpc, "ResolveToken: invalid token");
        reply->set_error(ErrorCodes::TOKEN_INVALID);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_uid(uid);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::debug(LogModule::Grpc, "ResolveToken: success uid={} cost={}ms", uid, cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::GetFileServer(ServerContext *context,
                                        const GetFileServerReq *request,
                                        GetFileServerRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    Log::info(LogModule::Grpc, "GetFileServer: uid={}", request->uid());

    if (request->uid() <= 0)
    {
        Log::warn(LogModule::Grpc, "GetFileServer: invalid uid={}", request->uid());
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    auto server = _registry->getNode("FileServer");
    if (!server)
    {
        Log::warn(LogModule::Grpc, "GetFileServer: no available file server for uid={}",
                  request->uid());
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    reply->set_host(server->client_host);
    reply->set_port(server->client_port);

    std::string token = utils::uuid::generate();
    reply->set_token(token);

    if (!_file_token_repo->saveFileToken(request->uid(), token, 60))
    {
        Log::error(LogModule::Grpc, "GetFileServer: failed to save file token for uid={}",
                   request->uid());
        reply->set_error(ErrorCodes::RPC_FAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc,
              "GetFileServer: assigned uid={} to {}:{} token={} cost={}ms",
              request->uid(), server->client_host, server->client_port,
              token, cost_ms);
    return Status::OK;
}

Status StatusServiceImpl::DeleteFileToken(ServerContext *context,
                                          const DeleteFileTokenReq *request,
                                          DeleteFileTokenRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "DeleteFileToken: uid={}", request->uid());

    if (request->uid() <= 0)
    {
        Log::warn(LogModule::Grpc, "DeleteFileToken: invalid uid={}", request->uid());
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    if (!_file_token_repo->deleteFileToken(request->uid()))
    {
        Log::warn(LogModule::Grpc, "DeleteFileToken: failed for uid={}", request->uid());
    }
    else
    {
        Log::info(LogModule::Grpc, "DeleteFileToken: uid={} token deleted", request->uid());
    }

    reply->set_error(ErrorCodes::SUCCESS);
    return Status::OK;
}
