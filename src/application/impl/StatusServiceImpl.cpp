// StatusServiceImpl.cpp — gRPC 服务实现
// 通过 NodeRegistry 接口转发所有节点注册/发现/绑定的业务逻辑
#include "StatusServiceImpl.h"

#include "Log.h"
#include "const.h"
#include "utils.h"

StatusServiceImpl::StatusServiceImpl(std::shared_ptr<NodeRegistry> registry,
                                     std::shared_ptr<GateNotifyClient> gate_client)
    : _registry(std::move(registry)), _gate_client(std::move(gate_client))
{
    Log::info(LogModule::Grpc, "StatusServiceImpl constructed, cleaning up expired nodes");
    _registry->cleanupExpiredNodes();
}

Status StatusServiceImpl::GetChatServer(ServerContext *context, const GetChatServerReq *request,
                                        GetChatServerRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "GetChatServer: uid={}", request->uid());

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
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_token(utils::generateUniqueString());
    insertToken(request->uid(), reply->token());

    Log::info(LogModule::Grpc, "GetChatServer: assigned uid={} to {}:{} with token", request->uid(),
              server->client_host, server->client_port);
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

Status StatusServiceImpl::RegisterChatNode(ServerContext *context,
                                           const RegisterChatNodeReq *request,
                                           RegisterChatNodeRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "RegisterChatNode: name={} instance={} client={}:{} rpc={}:{}",
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
        Log::error(LogModule::Grpc, "RegisterChatNode: failed to register node {}",
                   request->name());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_name(node.name);
    Log::info(LogModule::Grpc, "RegisterChatNode: node {} registered successfully",
              request->name());
    return Status::OK;
}

Status StatusServiceImpl::UnregisterChatNode(ServerContext *context,
                                             const UnregisterChatNodeReq *request,
                                             UnregisterChatNodeRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "UnregisterChatNode: name={} instance={}", request->name(),
              request->instance_id());

    if (!_registry->unregisterNode(request->name(), request->instance_id()))
    {
        Log::error(LogModule::Grpc, "UnregisterChatNode: failed to unregister node {}",
                   request->name());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    Log::info(LogModule::Grpc, "UnregisterChatNode: node {} unregistered successfully",
              request->name());
    return Status::OK;
}

Status StatusServiceImpl::HeartbeatChatNode(ServerContext *context,
                                            const HeartbeatChatNodeReq *request,
                                            HeartbeatChatNodeRsp *reply)
{
    (void)context;
    Log::debug(LogModule::Grpc, "HeartbeatChatNode: name={} instance={}", request->name(),
               request->instance_id());

    if (!_registry->heartbeat(request->name(), request->instance_id()))
    {
        Log::warn(LogModule::Grpc, "HeartbeatChatNode: failed for node {} instance {}",
                  request->name(), request->instance_id());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    return Status::OK;
}

Status StatusServiceImpl::GetUserChatNode(ServerContext *context, const GetUserChatNodeReq *request,
                                          GetUserChatNodeRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "GetUserChatNode: uid={}", request->uid());

    auto node = _registry->getNodeForUser(request->uid());
    if (!node)
    {
        Log::warn(LogModule::Grpc, "GetUserChatNode: no node found for uid={}", request->uid());
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_node_name(node->name);
    reply->set_rpc_host(node->rpc_host);
    reply->set_rpc_port(node->rpc_port);
    reply->set_client_host(node->client_host);
    reply->set_client_port(node->client_port);

    Log::info(LogModule::Grpc, "GetUserChatNode: uid={} -> node {} ({}:{})", request->uid(),
              node->name, node->client_host, node->client_port);
    return Status::OK;
}

Status StatusServiceImpl::BindUserToNode(ServerContext *context, const BindUserToNodeReq *request,
                                         BindUserToNodeRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "BindUserToNode: uid={} node={}", request->uid(),
              request->node_name());

    if (!_registry->bindUser(request->uid(), request->node_name()))
    {
        Log::error(LogModule::Grpc, "BindUserToNode: failed to bind uid={} to node {}",
                   request->uid(), request->node_name());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    Log::info(LogModule::Grpc, "BindUserToNode: uid={} bound to node {} successfully",
              request->uid(), request->node_name());
    return Status::OK;
}

Status StatusServiceImpl::UnbindUser(ServerContext *context, const UnbindUserReq *request,
                                     UnbindUserRsp *reply)
{
    (void)context;
    int uid = request->uid();
    Log::info(LogModule::Grpc, "UnbindUser: uid={}", uid);

    if (!_registry->unbindUser(uid))
    {
        Log::error(LogModule::Grpc, "UnbindUser: failed to unbind uid={}", uid);
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    // 通过 gRPC 通知 GateServer 用户下线
    if (_gate_client)
    {
        if (!_gate_client->notifyUserOffline(uid))
        {
            Log::warn(LogModule::Grpc, "UnbindUser: failed to notify GateServer for uid={}", uid);
        }
        else
        {
            Log::info(LogModule::Grpc, "UnbindUser: notified GateServer for uid={}", uid);
        }
    }

    reply->set_error(ErrorCodes::SUCCESS);
    Log::info(LogModule::Grpc, "UnbindUser: uid={} unbound successfully", uid);
    return Status::OK;
}

Status StatusServiceImpl::ValidateToken(ServerContext *context, const ValidateTokenReq *request,
                                        ValidateTokenRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "ValidateToken: uid={}", request->uid());

    int err = _registry->validateToken(request->uid(), request->token());
    if (err != ErrorCodes::SUCCESS)
    {
        reply->set_error(err);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_uid(request->uid());
    Log::info(LogModule::Grpc, "ValidateToken: success uid={}", request->uid());
    return Status::OK;
}
