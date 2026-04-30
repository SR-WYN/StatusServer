#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
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
    std::lock_guard<std::mutex> guard(_server_mutex);
    auto minServer = _servers.begin()->second;
    auto count_str = RedisMgr::getInstance().hGet(RedisPrefix::LOGIN_COUNT, minServer.name);
    if (count_str.empty())
    {
        // 不存在则默认设置为最大
        minServer.con_count = INT_MAX;
    }
    else
    {
        minServer.con_count = std::stoi(count_str);
    }

    // 使用范围基于for循环
    for (auto &server : _servers)
    {

        if (server.second.name == minServer.name)
        {
            continue;
        }

        auto count_str = RedisMgr::getInstance().hGet(RedisPrefix::LOGIN_COUNT, server.second.name);
        if (count_str.empty())
        {
            server.second.con_count = INT_MAX;
        }
        else
        {
            server.second.con_count = std::stoi(count_str);
        }

        if (server.second.con_count < minServer.con_count)
        {
            minServer = server.second;
        }
    }

    return minServer;
}

StatusServiceImpl::StatusServiceImpl()
{
    auto &cfg = ConfigMgr::getInstance();
    auto server_list = cfg["ChatServers"]["Name"];
    std::vector<std::string> words;
    std::stringstream ss(server_list);
    std::string word;
    while (std::getline(ss, word, ','))
    {
        words.push_back(word);
    }
    for (auto &word : words)
    {
        if (cfg[word]["Name"].empty())
        {
            continue;
        }
        ChatServer server;
        server.port = cfg[word]["Port"];
        server.host = cfg[word]["Host"];
        server.name = cfg[word]["Name"];
        server.con_count = 0;
        _servers.emplace(server.name, std::move(server));
    }
}

void StatusServiceImpl::insertToken(int uid, const std::string &token)
{
    std::string uid_str = std::to_string(uid);
    std::string token_key = RedisPrefix::USERTOKENPREFIX + uid_str;
    RedisMgr::getInstance().set(token_key, token);
}

Status StatusServiceImpl::Login(ServerContext *context, const LoginReq *request, LoginRsp *reply)
{
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