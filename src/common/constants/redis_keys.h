// redis_keys.h - Redis key 前缀与相关字段名常量
#pragma once

namespace constants::redis
{

// Redis key 前缀
constexpr const char *kCodePrefix = "code_";
constexpr const char *kUserTokenPrefix = "utoken_";
constexpr const char *kFileTokenPrefix = "ftoken_";
constexpr const char *kIpCountPrefix = "ipcount_";
constexpr const char *kUserBaseInfoPrefix = "ubaseinfo_";
constexpr const char *kLoginCountKey = "logincount";
constexpr const char *kRegisteredNodesKey = "registered_nodes";
constexpr const char *kUserNodePrefix = "user_node_";
constexpr const char *kNodeUsersPrefix = "node_users_";
constexpr const char *kUserOfflineChannel = "user_offline";

// 节点信息 JSON 字段名（用于 Redis 中存储的节点元数据）
constexpr const char *kExpireAt = "expire_at";
constexpr const char *kClientHost = "client_host";
constexpr const char *kClientPort = "client_port";
constexpr const char *kRpcHost = "rpc_host";
constexpr const char *kRpcPort = "rpc_port";
constexpr const char *kInstanceId = "instance_id";

} // namespace constants::redis