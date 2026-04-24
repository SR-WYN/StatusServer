#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "utils.h"
#include <algorithm>

Status StatusServiceImpl::GetChatServer(ServerContext *context, const GetChatServerReq *request,
                                        GetChatServerRsp *reply)
{
    (void)context;
    (void)request;

    ChatServer const server = getChatServer();
    if (server.host.empty() || server.port.empty())
    {
        reply->set_error(ErrorCodes::RPCFAILED);
        return Status::OK;
    }

    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_token(utils::generate_unique_string());
    insertToken(request->uid(), reply->token());
    return Status::OK;
}

ChatServer StatusServiceImpl::getChatServer()
{
    std::lock_guard<std::mutex> lock(_server_mutex);
    if (_servers.empty() || _connection_counts.size() != _servers.size())
    {
        return {};
    }

    // 选择当前连接数最小的 ChatServer（并列时返回先出现的节点）
    auto min_it = std::min_element(_connection_counts.begin(), _connection_counts.end());
    size_t const best = static_cast<size_t>(std::distance(_connection_counts.begin(), min_it));
    ++_connection_counts[best];
    return _servers[best];
}

StatusServiceImpl::StatusServiceImpl()
{
    auto &cfg = ConfigMgr::getInstance();
    ChatServer server;
    server.port = cfg["ChatServer1"]["Port"];
    server.host = cfg["ChatServer1"]["Host"];
    _servers.push_back(server);
    server.port = cfg["ChatServer2"]["Port"];
    server.host = cfg["ChatServer2"]["Host"];
    _servers.push_back(server);
    _connection_counts.assign(_servers.size(), 0);
}

void StatusServiceImpl::insertToken(int uid, const std::string &token)
{
    std::lock_guard<std::mutex> lock(_token_mutex);
    _tokens[uid] = token;
}

Status StatusServiceImpl::Login(ServerContext *context, const LoginReq *request, LoginRsp *reply)
{
    auto uid = request->uid();
    auto token = request->token();
    std::lock_guard<std::mutex> lock(_token_mutex);
    auto iter = _tokens.find(uid);
    if (iter == _tokens.end())
    {
        reply->set_error(ErrorCodes::UID_INVALID);
        return Status::OK;
    }
    if (iter->second != token)
    {
        reply->set_error(ErrorCodes::TOKEN_INVALID);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::SUCCESS);
    reply->set_uid(uid);
    reply->set_token(token);
    return Status::OK;
}