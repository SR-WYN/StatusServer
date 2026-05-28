#include "StatusServiceImpl.h"
#include "ChatNodeRegistry.h"
#include "RedisMgr.h"
#include "const.h"
#include "utils.h"

StatusServiceImpl::StatusServiceImpl()
{
    ChatNodeRegistry::purgeExpiredNodes();
}

void StatusServiceImpl::fillNodeReply(const RegisteredChatNode &node, GetChatNodeRsp *reply)
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

    auto server = ChatNodeRegistry::pickLeastLoadedNode();
    if (!server)
    {
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_host(server->client_host);
    reply->set_port(server->client_port);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_token(utils::generate_unique_string());
    insertToken(request->uid(), reply->token());
    return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, const std::string &token)
{
    std::string uid_str = std::to_string(uid);
    std::string token_key = RedisPrefix::USERTOKENPREFIX + uid_str;
    RedisMgr::getInstance().set(token_key, token);
}

Status StatusServiceImpl::Login(ServerContext *context, const LoginReq *request, LoginRsp *reply)
{
    (void)context;
    auto uid = request->uid();
    auto token = request->token();

    std::string uid_str = std::to_string(uid);
    std::string token_key = RedisPrefix::USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::getInstance().get(token_key, token_value);
    if (!success)
    {
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }

    if (token_value != token)
    {
        reply->set_error(ErrorCodes::TOKEN_INVALID);
        return Status::OK;
    }
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
    RegisteredChatNode node;
    node.name = request->name();
    node.client_host = request->client_host();
    node.client_port = request->client_port();
    node.rpc_host = request->rpc_host();
    node.rpc_port = request->rpc_port();
    node.instance_id = request->instance_id();
    if (!ChatNodeRegistry::registerNode(node))
    {
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_name(node.name);
    return Status::OK;
}

Status StatusServiceImpl::UnregisterChatNode(ServerContext *context,
                                             const UnregisterChatNodeReq *request,
                                             UnregisterChatNodeRsp *reply)
{
    (void)context;
    if (!ChatNodeRegistry::unregisterNode(request->name(), request->instance_id()))
    {
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    return Status::OK;
}

Status StatusServiceImpl::HeartbeatChatNode(ServerContext *context,
                                            const HeartbeatChatNodeReq *request,
                                            HeartbeatChatNodeRsp *reply)
{
    (void)context;
    if (!ChatNodeRegistry::heartbeat(request->name(), request->instance_id()))
    {
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
    auto node = ChatNodeRegistry::getUserNode(request->uid());
    if (!node)
    {
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_node_name(node->name);
    reply->set_rpc_host(node->rpc_host);
    reply->set_rpc_port(node->rpc_port);
    reply->set_client_host(node->client_host);
    reply->set_client_port(node->client_port);
    return Status::OK;
}

Status StatusServiceImpl::GetChatNode(ServerContext *context, const GetChatNodeReq *request,
                                      GetChatNodeRsp *reply)
{
    (void)context;
    auto node = ChatNodeRegistry::getNode(request->name());
    if (!node)
    {
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    fillNodeReply(*node, reply);
    return Status::OK;
}

Status StatusServiceImpl::BindUserToNode(ServerContext *context, const BindUserToNodeReq *request,
                                         BindUserToNodeRsp *reply)
{
    (void)context;
    if (!ChatNodeRegistry::bindUser(request->uid(), request->node_name()))
    {
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    return Status::OK;
}

Status StatusServiceImpl::UnbindUser(ServerContext *context, const UnbindUserReq *request,
                                     UnbindUserRsp *reply)
{
    (void)context;
    if (!ChatNodeRegistry::unbindUser(request->uid()))
    {
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    return Status::OK;
}
