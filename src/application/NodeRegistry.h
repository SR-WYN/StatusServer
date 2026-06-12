// NodeRegistry.h — 节点注册中心接口
// 定义 ChatServer 节点注册、发现与负载均衡的业务合约
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// 已注册的聊天节点信息
struct NodeInfo
{
    std::string name;        // 节点名称（唯一标识）
    std::string client_host; // 客户端连接地址
    std::string client_port; // 客户端连接端口
    std::string rpc_host;    // RPC 通信地址
    std::string rpc_port;    // RPC 通信端口
    std::string instance_id; // 实例 ID（区分同一节点的不同实例）
    int64_t expire_at = 0;   // 过期时间戳（秒），超时视为节点失活
};

/// 节点注册中心接口
/// 负责管理 ChatServer 节点的生命周期（注册/注销/心跳）、
/// 用户与节点的绑定关系，以及节点的负载均衡选择。
/// 不同的后端存储（Redis、内存等）可实现此接口。
class NodeRegistry
{
public:
    virtual ~NodeRegistry() = default;

    // 注册节点。若同名节点已存活且 instance_id 不同，则拒绝。
    virtual bool registerNode(const NodeInfo &node) = 0;

    // 注销节点，同时清理该节点绑定的所有用户路由。
    virtual bool unregisterNode(const std::string &name, const std::string &instance_id) = 0;

    // 刷新节点心跳，延长 expire_at。
    virtual bool heartbeat(const std::string &name, const std::string &instance_id) = 0;

    // 按名称查询存活节点，不存在或已过期返回 nullopt。
    virtual std::optional<NodeInfo> getNode(const std::string &name) = 0;

    // 查询用户当前绑定的节点。
    virtual std::optional<NodeInfo> getNodeForUser(int uid) = 0;

    // 列出所有存活节点。
    virtual std::vector<NodeInfo> listNodes() = 0;

    // 将用户绑定到指定节点。若用户之前已绑定在其他节点，先解绑旧关系。
    virtual bool bindUser(int uid, const std::string &node_name) = 0;

    // 解绑用户与节点的绑定关系。
    virtual bool unbindUser(int uid) = 0;

    // 在所有存活节点中选取登录用户数最少的节点（负载均衡）。
    virtual std::optional<NodeInfo> selectLeastLoadedNode() = 0;

    // 清理所有已过期的节点记录及其登录计数。
    virtual void cleanupExpiredNodes() = 0;
};
