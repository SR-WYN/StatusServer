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
                                     std::shared_ptr<GateNotifyClient> gate_client)
    : _registry(std::move(registry)),
      _file_token_repo(std::move(file_token_repo)),
      _gate_client(std::move(gate_client))
{
    Log::info(LogModule::Grpc, "StatusServiceImpl constructed, cleaning up expired nodes");
    _registry->cleanupExpiredNodes();
}

Status StatusServiceImpl::GetChatServer(ServerContext *context, const GetChatServerReq *request,
                                        GetChatServerRsp *reply)
{
    (void)context;
    const auto start = std::chrono::steady_clock::now();
    Log::info(LogModule::Grpc, "GetChatServer: uid={}", request->uid());

    if (request->uid() <= 0)
    {
        Log::warn(LogModule::Grpc, "GetChatServer: invalid uid={}", request->uid());
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    auto server = _registry->selectLeastLoadedNode();
    if (!server)
    {
        Log::warn(LogModule::Grpc, "GetChatServer: no available chat server for uid={}",
                  request->uid());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_host(server->client_host);
    reply->set_port(server->client_port);
    reply->set_token(utils::generateUniqueString());

    if (!_registry->saveToken(request->uid(), reply->token()))
    {
        Log::error(LogModule::Grpc, "GetChatServer: failed to save token for uid={}",
                   request->uid());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc,
              "GetChatServer: assigned uid={} to {}:{} token={} cost={}ms",
              request->uid(), server->client_host, server->client_port,
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
        reply->set_error(ErrorCodes::RPCFAILED);
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
        reply->set_error(ErrorCodes::RPCFAILED);
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
        reply->set_error(ErrorCodes::RPCFAILED);
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
        reply->set_error(ErrorCodes::RPCFAILED);
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

    if (!_registry->unbindUser(uid))
    {
        Log::warn(LogModule::Grpc, "UnbindUser: failed to unbind uid={}", uid);
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    // 通过 gRPC 通知 GateServer 用户下线（投递到 gRPC 客户端池，不阻塞当前线程）
    if (_gate_client)
    {
        int uidCopy = uid;
        auto gateClient = _gate_client;
        ThreadPoolMgr::getInstance().enqueueGrpcClient([gateClient, uidCopy]() {
            if (!gateClient->notifyUserOffline(uidCopy))
            {
                Log::warn(LogModule::Grpc, "UnbindUser: failed to notify GateServer for uid={}",
                          uidCopy);
            }
            else
            {
                Log::info(LogModule::Grpc, "UnbindUser: notified GateServer for uid={}", uidCopy);
            }
        });
    }
    else
    {
        Log::debug(LogModule::Grpc, "UnbindUser: no gate client configured, skip notification");
    }

    reply->set_error(ErrorCodes::SUCCESS);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Grpc, "UnbindUser: uid={} unbound cost={}ms", uid, cost_ms);
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
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_host(server->client_host);
    reply->set_port(server->client_port);

    std::string token = utils::generateUniqueString();
    reply->set_token(token);

    if (!_file_token_repo->saveFileToken(request->uid(), token, 60))
    {
        Log::error(LogModule::Grpc, "GetFileServer: failed to save file token for uid={}",
                   request->uid());
        reply->set_error(ErrorCodes::RPCFAILED);
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
