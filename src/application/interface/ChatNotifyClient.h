// ChatNotifyClient.h - ChatServer 通知客户端接口
// StatusServer 通过此接口通知 ChatServer 踢掉指定用户
#pragma once

#include <string>

/// ChatServer 通知客户端接口
class ChatNotifyClient
{
public:
    virtual ~ChatNotifyClient() = default;

    /// 通知指定 ChatServer 踢掉用户
    /// @param rpc_host ChatServer RPC 地址
    /// @param rpc_port ChatServer RPC 端口
    /// @param uid 用户 ID
    /// @param reason 踢人原因
    /// @return 是否通知成功
    virtual bool notifyKickUser(const std::string &rpc_host, const std::string &rpc_port,
                                int uid, const std::string &reason) = 0;
};
