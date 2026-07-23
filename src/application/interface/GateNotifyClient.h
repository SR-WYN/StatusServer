// GateNotifyClient.h - GateServer 通知客户端接口
// StatusServer 通过此接口通知 GateServer 用户下线事件
#pragma once

/// GateServer 通知客户端接口
class GateNotifyClient
{
public:
    virtual ~GateNotifyClient() = default;

    /// 通知 GateServer 用户已下线
    /// @param uid 用户 ID
    /// @return 是否通知成功
    virtual bool notifyUserOffline(int uid) = 0;

    /// 通知 GateServer 用户已重新上线（刷新 session TTL）
    /// @param uid 用户 ID
    /// @return 是否通知成功
    virtual bool notifyUserOnline(int uid) = 0;
};
