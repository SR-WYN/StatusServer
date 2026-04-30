#pragma once
#include "Singleton.h"
#include "data.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <json/json.h>
#include <memory>
#include <queue>
#include <unordered_map>

using grpc::ClientContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::ChatService;
using message::TextChatData;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

class ChatConPool;

class ChatGrpcClient : public Singleton<ChatGrpcClient>
{
    friend class Singleton<ChatGrpcClient>;

public:
    ~ChatGrpcClient() override;
    AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq &req);
    AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq &req);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> &user_info);
    TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq &req, const Json::Value &root_value);

private:
    ChatGrpcClient();
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};
