#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include <memory>
#include "ChatConPool.h"

ChatGrpcClient::ChatGrpcClient()
{
    auto& cfg = ConfigMgr::getInstance();
    auto server_list = cfg["PeerServer"]["Servers"];
    std::vector<std::string> words;
    std::stringstream ss(server_list);
    std::string word;

    while (std::getline(ss, word, ','))
    {
        words.push_back(word);
    }
    for (auto& word : words)
    {
        if (cfg[word]["Name"].empty())
        {
            continue;
        }
        _pools[cfg[word]["Name"]] = std::make_unique<ChatConPool>(5,cfg[word]["Host"],cfg[word]["Port"]);
    }
}

ChatGrpcClient::~ChatGrpcClient()
{
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip,const AddFriendReq& req)
{
    return AddFriendRsp();
    //todo
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip,const AuthFriendReq& req)
{
    return AuthFriendRsp();
    //todo
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key,int uid,std::shared_ptr<UserInfo>& user_info)
{
    return false;
    //todo
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip,const TextChatMsgReq& req,const Json::Value& root_value)
{
    return TextChatMsgRsp();
    //todo
}
