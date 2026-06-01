#include "StatusServiceImpl.h"
#include "Log.h"
#include "NodeRegistry.h"
#include "RedisMgr.h"
#include "const.h"
#include "utils.h"

StatusServiceImpl::StatusServiceImpl()
{
    Log::info(LogModule::Grpc, "StatusServiceImpl constructed, cleaning up expired nodes");
    NodeRegistry::cleanupExpiredNodes();
}

void StatusServiceImpl::fillNodeReply(const NodeInfo &node, GetChatNodeRsp *reply)
{
    reply->set_name(node.name);
    reply->set_rpc_host(node.rpc_host);
    reply->set_rpc_port(node.rpc_port);
    reply->set_client_host(node.client_host);
    reply->set_client_port(node.client_port);
}

Status StatusServiceImpl::GetChatServer(ServerContext *context, const GetChatServerReq *request,
                                        GetChatServerRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "GetChatServer: uid={}", request->uid());

    auto server = NodeRegistry::selectLeastLoadedNode();
    if (!server)
    {
        Log::warn(LogModule::Grpc, "GetChatServer: no available chat server for uid={}", request->uid());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_host(server->client_host);
    reply->set_port(server->client_port);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_token(utils::generate_unique_string());
    insertToken(request->uid(), reply->token());
    Log::info(LogModule::Grpc,
              "GetChatServer: assigned uid={} to {}:{} with token",
              request->uid(), server->client_host, server->client_port);
    return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, const std::string &token)
{
    std::string uid_str = std::to_string(uid);
    std::string token_key = RedisPrefix::USERTOKENPREFIX + uid_str;
    RedisMgr::getInstance().set(token_key, token);
    Log::debug(LogModule::Grpc, "insertToken: uid={} token={}", uid, token);
}

Status StatusServiceImpl::Login(ServerContext *context, const LoginReq *request, LoginRsp *reply)
{
    (void)context;
    auto uid = request->uid();
    auto token = request->token();
    Log::info(LogModule::Grpc, "Login: uid={} token={}", uid, token);

    std::string uid_str = std::to_string(uid);
    std::string token_key = RedisPrefix::USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::getInstance().get(token_key, token_value);
    if (!success)
    {
        Log::warn(LogModule::Grpc, "Login: uid={} not found in Redis", uid);
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    if (token_value != token)
    {
        Log::warn(LogModule::Grpc,
                  "Login: token mismatch for uid={} (expected={}, got={})",
                  uid, token_value, token);
        reply->set_error(ErrorCodes::TOKEN_INVALID);
        return Status::OK;
    }
    Log::info(LogModule::Grpc, "Login: uid={} login success", uid);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_uid(uid);
    reply->set_token(token);
    return Status::OK;
}

Status StatusServiceImpl::RegisterChatNode(ServerContext *context,
                                           const RegisterChatNodeReq *request,
                                           RegisterChatNodeRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc,
              "RegisterChatNode: name={} instance={} client={}:{} rpc={}:{}",
              request->name(), request->instance_id(),
              request->client_host(), request->client_port(),
              request->rpc_host(), request->rpc_port());

    NodeInfo node;
    node.name = request->name();
    node.client_host = request->client_host();
    node.client_port = request->client_port();
    node.rpc_host = request->rpc_host();
    node.rpc_port = request->rpc_port();
    node.instance_id = request->instance_id();
    if (!NodeRegistry::registerNode(node))
    {
        Log::error(LogModule::Grpc, "RegisterChatNode: failed to register node {}", request->name());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_name(node.name);
    Log::info(LogModule::Grpc, "RegisterChatNode: node {} registered successfully", request->name());
    return Status::OK;
}

Status StatusServiceImpl::UnregisterChatNode(ServerContext *context,
                                             const UnregisterChatNodeReq *request,
                                             UnregisterChatNodeRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc,
              "UnregisterChatNode: name={} instance={}",
              request->name(), request->instance_id());

    if (!NodeRegistry::unregisterNode(request->name(), request->instance_id()))
    {
        Log::error(LogModule::Grpc, "UnregisterChatNode: failed to unregister node {}", request->name());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    Log::info(LogModule::Grpc, "UnregisterChatNode: node {} unregistered successfully", request->name());
    return Status::OK;
}

Status StatusServiceImpl::HeartbeatChatNode(ServerContext *context,
                                            const HeartbeatChatNodeReq *request,
                                            HeartbeatChatNodeRsp *reply)
{
    (void)context;
    Log::debug(LogModule::Grpc,
               "HeartbeatChatNode: name={} instance={}",
               request->name(), request->instance_id());

    if (!NodeRegistry::heartbeat(request->name(), request->instance_id()))
    {
        Log::warn(LogModule::Grpc,
                  "HeartbeatChatNode: failed for node {} instance {}",
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

    auto node = NodeRegistry::getNodeForUser(request->uid());
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
    Log::info(LogModule::Grpc,
              "GetUserChatNode: uid={} -> node {} ({}:{})",
              request->uid(), node->name, node->client_host, node->client_port);
    return Status::OK;
}

Status StatusServiceImpl::GetChatNode(ServerContext *context, const GetChatNodeReq *request,
                                      GetChatNodeRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "GetChatNode: name={}", request->name());

    auto node = NodeRegistry::getNode(request->name());
    if (!node)
    {
        Log::warn(LogModule::Grpc, "GetChatNode: node {} not found", request->name());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    fillNodeReply(*node, reply);
    Log::info(LogModule::Grpc,
              "GetChatNode: name={} client={}:{} rpc={}:{}",
              node->name, node->client_host, node->client_port,
              node->rpc_host, node->rpc_port);
    return Status::OK;
}

Status StatusServiceImpl::BindUserToNode(ServerContext *context, const BindUserToNodeReq *request,
                                         BindUserToNodeRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc,
              "BindUserToNode: uid={} node={}", request->uid(), request->node_name());

    if (!NodeRegistry::bindUser(request->uid(), request->node_name()))
    {
        Log::error(LogModule::Grpc,
                   "BindUserToNode: failed to bind uid={} to node {}",
                   request->uid(), request->node_name());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    Log::info(LogModule::Grpc,
              "BindUserToNode: uid={} bound to node {} successfully",
              request->uid(), request->node_name());
    return Status::OK;
}

Status StatusServiceImpl::UnbindUser(ServerContext *context, const UnbindUserReq *request,
                                     UnbindUserRsp *reply)
{
    (void)context;
    Log::info(LogModule::Grpc, "UnbindUser: uid={}", request->uid());

    if (!NodeRegistry::unbindUser(request->uid()))
    {
        Log::error(LogModule::Grpc, "UnbindUser: failed to unbind uid={}", request->uid());
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    Log::info(LogModule::Grpc, "UnbindUser: uid={} unbound successfully", request->uid());
    return Status::OK;
}
