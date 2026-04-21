#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "utils.h"

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request,
                                        GetChatServerRsp* reply)
{
    std::string prefix("loadbalancer status server has received :  ");
    _server_index = (_server_index++) % (_servers.size());
    auto& server = _servers[_server_index];
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_token(utils::generate_unique_string());
    return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
    : _server_index(0)
{
    auto& cfg = ConfigMgr::GetInstance();
    ChatServer server;
    server.port = cfg["ChatServer1"]["Port"];
    server.host = cfg["ChatServer1"]["Host"];
    _servers.push_back(server);
    server.port = cfg["ChatServer2"]["Port"];
    server.host = cfg["ChatServer2"]["Host"];
    _servers.push_back(server);
}