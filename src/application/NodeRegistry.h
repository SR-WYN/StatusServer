#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// 已注册的聊天节点信息
struct NodeInfo
{
    std::string name;         // 节点名称（唯一标识）
    std::string client_host;  // 客户端连接地址
    std::string client_port;  // 客户端连接端口
    std::string rpc_host;     // RPC 通信地址
    std::string rpc_port;     // RPC 通信端口
    std::string instance_id;  // 实例 ID（区分同一节点的不同实例）
    int64_t expire_at = 0;    // 过期时间戳（秒），超时视为节点失活
};

/// 节点注册中心 —— 基于 Redis 管理 ChatServer 节点的注册、发现与负载均衡
class NodeRegistry
{
public:
    // 注册节点（若同名节点已存活且 instance_id 不同则拒绝）
    static bool registerNode(const NodeInfo &node);
    // 注销节点，同时清理其绑定的用户路由
    static bool unregisterNode(const std::string &name, const std::string &instance_id);
    // 节点心跳续期，刷新 expire_at
    static bool heartbeat(const std::string &name, const std::string &instance_id);
    // 按名称查询存活节点
    static std::optional<NodeInfo> getNode(const std::string &name);
    // 查询用户当前绑定的节点
    static std::optional<NodeInfo> getNodeForUser(int uid);
    // 列出所有存活节点
    static std::vector<NodeInfo> listNodes();
    // 将用户绑定到指定节点
    static bool bindUser(int uid, const std::string &node_name);
    // 解绑用户与节点的绑定关系
    static bool unbindUser(int uid);
    // 选取登录数最少的节点（负载均衡）
    static std::optional<NodeInfo> selectLeastLoadedNode();
    // 清理已过期的节点记录
    static void cleanupExpiredNodes();

private:
    static std::string serializeNode(const NodeInfo &node);
    static bool parseNode(const std::string &json, NodeInfo &out);
    static bool isAlive(const NodeInfo &node);
};
