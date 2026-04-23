#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "utils.h"

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request,
                                        GetChatServerRsp* reply)
{
    (void)context;
    (void)request;

    std::lock_guard<std::mutex> lock(_server_mutex);
    if (_servers.empty())
    {
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    // 先取当前索引，再将索引向后轮询推进。
    auto& server = _servers[_server_index];
    _server_index = (_server_index + 1) % static_cast<int>(_servers.size());
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_token(utils::generate_unique_string());
    return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
    : _server_index(0)
{
    auto& cfg = ConfigMgr::getInstance();
    ChatServer server;
    server.port = cfg["ChatServer1"]["Port"];
    server.host = cfg["ChatServer1"]["Host"];
    _servers.push_back(server);
    server.port = cfg["ChatServer2"]["Port"];
    server.host = cfg["ChatServer2"]["Host"];
    _servers.push_back(server);
}