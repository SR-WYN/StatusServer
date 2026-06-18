// RedisNodeRegistryImpl.h — Redis 实现的节点注册中心
// 将节点元数据、用户绑定、登录计数存储在 Redis 中
#pragma once

#include "NodeRegistry.h"
#include <string>

// Redis 实现的节点注册中心。
// 节点信息以 JSON 格式存储在 Redis Hash 中（key: chat_nodes, field: 节点名），
// 用户与节点的绑定关系通过 KV + Set 结构维护。
class RedisNodeRegistryImpl : public NodeRegistry
{
public:
    RedisNodeRegistryImpl() = default;
    ~RedisNodeRegistryImpl() override = default;

    // 禁用拷贝和赋值
    RedisNodeRegistryImpl(const RedisNodeRegistryImpl &) = delete;
    RedisNodeRegistryImpl &operator=(const RedisNodeRegistryImpl &) = delete;

    // ---- NodeRegistry 接口实现 ----
    bool registerNode(const NodeInfo &node) override;
    bool unregisterNode(const std::string &name, const std::string &instance_id) override;
    bool heartbeat(const std::string &name, const std::string &instance_id) override;
    std::optional<NodeInfo> getNode(const std::string &name) override;
    std::optional<NodeInfo> getNodeForUser(int uid) override;
    std::vector<NodeInfo> listNodes() override;
    bool bindUser(int uid, const std::string &node_name) override;
    bool unbindUser(int uid) override;
    std::optional<NodeInfo> selectLeastLoadedNode() override;
    void cleanupExpiredNodes() override;
    bool saveToken(int uid, const std::string &token) override;
    int validateToken(int uid, const std::string &token) override;

private:
    // 将 NodeInfo 序列化为 JSON 字符串
    static std::string serializeNode(const NodeInfo &node);
    // 从 JSON 字符串反序列化 NodeInfo，返回是否成功
    static bool parseNode(const std::string &json, NodeInfo &out);
    // 判断节点是否存活（未过期）
    static bool isAlive(const NodeInfo &node);
};
