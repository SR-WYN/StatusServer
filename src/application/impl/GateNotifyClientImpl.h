// GateNotifyClientImpl.h - GateServer 通知客户端实现
#pragma once

#include "GateNotifyClient.h"

#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

/// GateServer 通知客户端实现
class GateNotifyClientImpl : public GateNotifyClient
{
public:
    /// 构造函数
    /// @param address GateServer gRPC 服务地址，如 "127.0.0.1:51052"
    explicit GateNotifyClientImpl(const std::string &address);

    /// 通知 GateServer 用户已下线
    /// @param uid 用户 ID
    /// @return 是否通知成功
    bool notifyUserOffline(int uid) override;

    /// 通知 GateServer 用户已重新上线（刷新 session TTL）
    /// @param uid 用户 ID
    /// @return 是否通知成功
    bool notifyUserOnline(int uid) override;

private:
    std::unique_ptr<message::GateNotifyService::Stub> _stub;
};
