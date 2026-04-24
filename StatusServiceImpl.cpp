#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "utils.h"
#include <algorithm>

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

    // 选择当前分配计数最少的 Chat 节点（并列时取靠前下标），再为其累加计数。
    auto min_it = std::min_element(_connection_counts.begin(), _connection_counts.end());
    size_t const best = static_cast<size_t>(std::distance(_connection_counts.begin(), min_it));
    ++_connection_counts[best];
    ChatServer const& server = _servers[best];
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_token(utils::generate_unique_string());
    return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
{
    auto& cfg = ConfigMgr::getInstance();
    ChatServer server;
    server.port = cfg["ChatServer1"]["Port"];
    server.host = cfg["ChatServer1"]["Host"];
    _servers.push_back(server);
    server.port = cfg["ChatServer2"]["Port"];
    server.host = cfg["ChatServer2"]["Host"];
    _servers.push_back(server);
    _connection_counts.assign(_servers.size(), 0);
}