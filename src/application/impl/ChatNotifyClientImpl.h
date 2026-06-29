// ChatNotifyClientImpl.h - ChatServer 通知客户端实现
#pragma once

#include "ChatNotifyClient.h"

#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

/// ChatServer 通知客户端实现（带 stub 缓存）
class ChatNotifyClientImpl : public ChatNotifyClient
{
public:
    ChatNotifyClientImpl() = default;
    ~ChatNotifyClientImpl() override = default;

    /// 通知指定 ChatServer 踢掉用户
    bool notifyKickUser(const std::string &rpc_host, const std::string &rpc_port,
                        int uid, const std::string &reason) override;

private:
    std::shared_ptr<message::ChatService::Stub> getOrCreateStub(
        const std::string &rpc_host, const std::string &rpc_port);

    std::mutex _mutex;
    std::unordered_map<std::string, std::shared_ptr<message::ChatService::Stub>> _stubs;
};
