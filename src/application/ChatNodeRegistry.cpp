#include "ChatNodeRegistry.h"
#include "RedisMgr.h"
#include "const.h"
#include <chrono>
#include <climits>
#include <json/reader.h>
#include <json/writer.h>
#include <sstream>

namespace
{
constexpr const char *kExpireAt = "expire_at";
constexpr const char *kClientHost = "client_host";
constexpr const char *kClientPort = "client_port";
constexpr const char *kRpcHost = "rpc_host";
constexpr const char *kRpcPort = "rpc_port";
constexpr const char *kInstanceId = "instance_id";
} // namespace

int64_t ChatNodeRegistry::nowSec()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string ChatNodeRegistry::serializeNode(const RegisteredChatNode &node)
{
    Json::Value root;
    root[kClientHost] = node.client_host;
    root[kClientPort] = node.client_port;
    root[kRpcHost] = node.rpc_host;
    root[kRpcPort] = node.rpc_port;
    root[kInstanceId] = node.instance_id;
    root[kExpireAt] = static_cast<Json::Int64>(node.expire_at);
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

bool ChatNodeRegistry::parseNode(const std::string &json, RegisteredChatNode &out)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(json, root) || !root.isObject())
    {
        return false;
    }
    out.client_host = root.get(kClientHost, "").asString();
    out.client_port = root.get(kClientPort, "").asString();
    out.rpc_host = root.get(kRpcHost, "").asString();
    out.rpc_port = root.get(kRpcPort, "").asString();
    out.instance_id = root.get(kInstanceId, "").asString();
    out.expire_at = root.get(kExpireAt, static_cast<Json::Int64>(0)).asInt64();
    return !out.client_host.empty() && !out.client_port.empty() && !out.rpc_host.empty() &&
           !out.rpc_port.empty();
}

bool ChatNodeRegistry::isAlive(const RegisteredChatNode &node)
{
    return node.expire_at >= nowSec();
}

void ChatNodeRegistry::purgeExpiredNodes()
{
    std::map<std::string, std::string> all;
    if (!RedisMgr::getInstance().hGetAll(RedisPrefix::CHAT_NODES, all))
    {
        return;
    }
    for (const auto &entry : all)
    {
        RegisteredChatNode node;
        node.name = entry.first;
        if (!parseNode(entry.second, node) || !isAlive(node))
        {
            RedisMgr::getInstance().hDel(RedisPrefix::CHAT_NODES, entry.first);
            RedisMgr::getInstance().hDel(RedisPrefix::LOGIN_COUNT, entry.first);
            std::cout << "Purged expired chat node: " << entry.first << std::endl;
        }
    }
}

bool ChatNodeRegistry::registerNode(const RegisteredChatNode &node)
{
    if (node.name.empty())
    {
        return false;
    }
    purgeExpiredNodes();
    auto existing = getNode(node.name);
    if (existing && isAlive(*existing) && existing->instance_id != node.instance_id)
    {
        return false;
    }

    RegisteredChatNode stored = node;
    stored.expire_at = nowSec() + CHAT_NODE_TTL_SEC;
    auto &redis = RedisMgr::getInstance();
    if (!redis.hSet(RedisPrefix::CHAT_NODES, node.name, serializeNode(stored)))
    {
        return false;
    }
    redis.hSet(RedisPrefix::LOGIN_COUNT, node.name, "0");
    return true;
}

bool ChatNodeRegistry::unregisterNode(const std::string &name, const std::string &instance_id)
{
    auto existing = getNode(name);
    if (!existing)
    {
        return true;
    }
    if (!instance_id.empty() && existing->instance_id != instance_id)
    {
        return false;
    }

    auto &redis = RedisMgr::getInstance();
    const std::string users_key = std::string(RedisPrefix::CHAT_NODE_USERS) + name;
    std::vector<std::string> uids;
    redis.sMembers(users_key, uids);
    for (const auto &uid_str : uids)
    {
        redis.del(std::string(RedisPrefix::USER_CHAT_NODE) + uid_str);
        redis.del(std::string(RedisPrefix::USERIPPREFIX) + uid_str);
    }
    redis.del(users_key);
    redis.hDel(RedisPrefix::CHAT_NODES, name);
    redis.hDel(RedisPrefix::LOGIN_COUNT, name);
    return true;
}

bool ChatNodeRegistry::heartbeat(const std::string &name, const std::string &instance_id)
{
    auto existing = getNode(name);
    if (!existing || existing->instance_id != instance_id)
    {
        return false;
    }
    RegisteredChatNode updated = *existing;
    updated.expire_at = nowSec() + CHAT_NODE_TTL_SEC;
    return RedisMgr::getInstance().hSet(RedisPrefix::CHAT_NODES, name, serializeNode(updated));
}

std::optional<RegisteredChatNode> ChatNodeRegistry::getNode(const std::string &name)
{
    const std::string json =
        RedisMgr::getInstance().hGet(RedisPrefix::CHAT_NODES, name);
    if (json.empty())
    {
        return std::nullopt;
    }
    RegisteredChatNode node;
    node.name = name;
    if (!parseNode(json, node) || !isAlive(node))
    {
        return std::nullopt;
    }
    return node;
}

std::vector<RegisteredChatNode> ChatNodeRegistry::listAliveNodes()
{
    std::vector<RegisteredChatNode> result;
    std::map<std::string, std::string> all;
    if (!RedisMgr::getInstance().hGetAll(RedisPrefix::CHAT_NODES, all))
    {
        return result;
    }
    for (const auto &entry : all)
    {
        RegisteredChatNode node;
        node.name = entry.first;
        if (parseNode(entry.second, node) && isAlive(node))
        {
            result.push_back(std::move(node));
        }
    }
    return result;
}

std::optional<RegisteredChatNode> ChatNodeRegistry::getUserNode(int uid)
{
    std::string node_name;
    if (!RedisMgr::getInstance().get(std::string(RedisPrefix::USER_CHAT_NODE) + std::to_string(uid),
                                     node_name) ||
        node_name.empty())
    {
        return std::nullopt;
    }
    return getNode(node_name);
}

bool ChatNodeRegistry::bindUser(int uid, const std::string &node_name)
{
    if (uid <= 0 || node_name.empty())
    {
        return false;
    }
    // 节点可能刚注册，getNode 校验失败时仍允许绑定（路由表由 chat_nodes 维护）
    if (!getNode(node_name))
    {
        std::cerr << "bindUser: node not in alive registry, binding anyway name=" << node_name
                  << std::endl;
    }
    const std::string uid_str = std::to_string(uid);
    auto &redis = RedisMgr::getInstance();
    std::string old_node;
    if (redis.get(std::string(RedisPrefix::USER_CHAT_NODE) + uid_str, old_node) && !old_node.empty() &&
        old_node != node_name)
    {
        redis.sRem(std::string(RedisPrefix::CHAT_NODE_USERS) + old_node, uid_str);
    }
    redis.set(std::string(RedisPrefix::USER_CHAT_NODE) + uid_str, node_name);
    redis.set(std::string(RedisPrefix::USERIPPREFIX) + uid_str, node_name);
    redis.sAdd(std::string(RedisPrefix::CHAT_NODE_USERS) + node_name, uid_str);
    return true;
}

bool ChatNodeRegistry::unbindUser(int uid)
{
    if (uid <= 0)
    {
        return false;
    }
    const std::string uid_str = std::to_string(uid);
    auto &redis = RedisMgr::getInstance();
    std::string node_name;
    if (redis.get(std::string(RedisPrefix::USER_CHAT_NODE) + uid_str, node_name) && !node_name.empty())
    {
        redis.sRem(std::string(RedisPrefix::CHAT_NODE_USERS) + node_name, uid_str);
    }
    redis.del(std::string(RedisPrefix::USER_CHAT_NODE) + uid_str);
    redis.del(std::string(RedisPrefix::USERIPPREFIX) + uid_str);
    return true;
}

std::optional<RegisteredChatNode> ChatNodeRegistry::pickLeastLoadedNode()
{
    auto nodes = listAliveNodes();
    if (nodes.empty())
    {
        return std::nullopt;
    }
    RegisteredChatNode best = nodes.front();
    int best_count = INT_MAX;
    for (const auto &node : nodes)
    {
        auto count_str = RedisMgr::getInstance().hGet(RedisPrefix::LOGIN_COUNT, node.name);
        int count = count_str.empty() ? INT_MAX : std::stoi(count_str);
        if (count < best_count)
        {
            best_count = count;
            best = node;
        }
    }
    return best;
}
