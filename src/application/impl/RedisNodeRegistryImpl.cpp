// RedisNodeRegistryImpl.cpp - Redis 节点注册中心实现
// 从原 NodeRegistry.cpp 迁移，去掉 static 关键字，改为实例方法
#include "RedisNodeRegistryImpl.h"
#include "Log.h"
#include "RedisMgr.h"
#include "business_constants.h"
#include "const.h"
#include "redis_keys.h"
#include "utils.h"

#include <chrono>
#include <climits>
#include <json/reader.h>
#include <json/writer.h>
#include <sstream>

// 将节点信息序列化为 JSON 字符串
std::string RedisNodeRegistryImpl::serializeNode(const NodeInfo &node)
{
    Json::Value root;
    root[constants::redis::kClientHost] = node.client_host;
    root[constants::redis::kClientPort] = node.client_port;
    root[constants::redis::kRpcHost] = node.rpc_host;
    root[constants::redis::kRpcPort] = node.rpc_port;
    root[constants::redis::kInstanceId] = node.instance_id;
    root[constants::redis::kExpireAt] = static_cast<Json::Int64>(node.expire_at);
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

// 从 JSON 字符串反序列化节点信息，返回是否成功
bool RedisNodeRegistryImpl::parseNode(const std::string &json, NodeInfo &out)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(json, root) || !root.isObject())
    {
        std::string preview = json.substr(0, std::min<size_t>(200, json.size()));
        if (json.size() > 200)
        {
            preview += "...";
        }
        Log::warn(LogModule::Registry, "parseNode: failed to parse node JSON ({} bytes): {}",
                  json.size(), preview);
        return false;
    }
    out.client_host = root.get(constants::redis::kClientHost, "").asString();
    out.client_port = root.get(constants::redis::kClientPort, "").asString();
    out.rpc_host = root.get(constants::redis::kRpcHost, "").asString();
    out.rpc_port = root.get(constants::redis::kRpcPort, "").asString();
    out.instance_id = root.get(constants::redis::kInstanceId, "").asString();
    out.expire_at = root.get(constants::redis::kExpireAt, static_cast<Json::Int64>(0)).asInt64();
    return !out.client_host.empty() && !out.client_port.empty() && !out.rpc_host.empty() &&
           !out.rpc_port.empty();
}

// 判断节点是否存活（未过期）
bool RedisNodeRegistryImpl::isAlive(const NodeInfo &node)
{
    return node.expire_at >= utils::time::nowSec();
}

// 保存用户 token 到 Redis，设置短 TTL
// 主数据：utoken:{token} -> Hash{uid, token}
// 查找索引：utoken_{uid} -> token（用于 deleteToken / refreshTokenTTL）
bool RedisNodeRegistryImpl::saveToken(int uid, const std::string &token)
{
    if (token.empty())
    {
        Log::warn(LogModule::Registry, "saveToken: empty token for uid={}", uid);
        return false;
    }

    std::string uidStr = std::to_string(uid);
    std::string tokenKey = std::string(constants::redis::kUserTokenPrefix) + token;
    std::string lookupKey = std::string(constants::redis::kUserTokenLookupPrefix) + uidStr;

    auto &redis = RedisMgr::getInstance();

    // 先写主 Hash key
    bool ok = redis.hSet(tokenKey, "uid", uidStr) &&
              redis.hSet(tokenKey, "token", token) &&
              redis.expire(tokenKey, constants::business::kTokenTtlSeconds);
    if (!ok)
    {
        Log::warn(LogModule::Registry, "saveToken: failed to write token key uid={} key={}",
                  uid, tokenKey);
        redis.del(tokenKey);
        return false;
    }

    // 再写 uid -> token 查找索引
    if (!redis.setEx(lookupKey, token, constants::business::kTokenTtlSeconds))
    {
        Log::warn(LogModule::Registry, "saveToken: failed to write lookup key uid={} key={}",
                  uid, lookupKey);
        // 回滚主 key，避免只有认证数据没有查找索引
        redis.del(tokenKey);
        return false;
    }

    Log::debug(LogModule::Registry, "saveToken: uid={} token={} ttl={}s", uid, token,
               constants::business::kTokenTtlSeconds);
    return true;
}

// 验证用户 token（同时支持登录 token 和文件传输临时 token）
int RedisNodeRegistryImpl::validateToken(int uid, const std::string &token)
{
    if (token.empty())
    {
        Log::warn(LogModule::Registry, "validateToken: empty token for uid={}", uid);
        return ErrorCodes::TOKEN_INVALID;
    }

    // 1. 先检查以 token 为主 key 的登录 token
    std::string tokenKey = std::string(constants::redis::kUserTokenPrefix) + token;
    std::map<std::string, std::string> fields;
    if (RedisMgr::getInstance().hGetAll(tokenKey, fields))
    {
        auto itUid = fields.find("uid");
        auto itToken = fields.find("token");
        if (itUid != fields.end() && itToken != fields.end() &&
            itUid->second == std::to_string(uid) && itToken->second == token)
        {
            Log::debug(LogModule::Registry, "validateToken: user token matched uid={}", uid);
            return ErrorCodes::SUCCESS;
        }
    }

    // 2. 再检查文件传输临时 token（仍按 uid 索引）
    std::string uidStr = std::to_string(uid);
    std::string fileTokenKey = std::string(constants::redis::kFileTokenPrefix) + uidStr;
    std::string storedToken;
    if (RedisMgr::getInstance().get(fileTokenKey, storedToken) && storedToken == token)
    {
        Log::debug(LogModule::Registry, "validateToken: file token matched uid={}", uid);
        return ErrorCodes::SUCCESS;
    }

    Log::warn(LogModule::Registry, "validateToken: failed uid={} token={}", uid, token);
    return ErrorCodes::TOKEN_INVALID;
}

// 通过 token 反查 uid
int RedisNodeRegistryImpl::resolveToken(const std::string &token)
{
    if (token.empty())
    {
        Log::warn(LogModule::Registry, "resolveToken: empty token");
        return 0;
    }

    std::string tokenKey = std::string(constants::redis::kUserTokenPrefix) + token;
    std::string uidStr = RedisMgr::getInstance().hGet(tokenKey, "uid");
    if (uidStr.empty())
    {
        Log::warn(LogModule::Registry, "resolveToken: token not found key={}", tokenKey);
        return 0;
    }

    try
    {
        int uid = std::stoi(uidStr);
        Log::debug(LogModule::Registry, "resolveToken: token -> uid={}", uid);
        return uid;
    }
    catch (const std::exception &e)
    {
        Log::error(LogModule::Registry, "resolveToken: invalid uid string uidStr={} err={}",
                   uidStr, e.what());
        return 0;
    }
}

// 清理 Redis 中所有已过期的节点记录及其登录计数
void RedisNodeRegistryImpl::cleanupExpiredNodes()
{
    const auto start = std::chrono::steady_clock::now();
    Log::debug(LogModule::Registry, "cleanupExpiredNodes: scanning started");

    std::map<std::string, std::string> all;
    if (!RedisMgr::getInstance().hGetAll(constants::redis::kRegisteredNodesKey, all))
    {
        Log::warn(LogModule::Registry, "cleanupExpiredNodes: failed to get all nodes from Redis");
        return;
    }

    int cleaned = 0;
    int alive = 0;
    for (const auto &entry : all)
    {
        NodeInfo node;
        node.name = entry.first;
        if (!parseNode(entry.second, node))
        {
            Log::warn(LogModule::Registry,
                      "cleanupExpiredNodes: invalid node JSON for '{}', removing", entry.first);
            RedisMgr::getInstance().hDel(constants::redis::kRegisteredNodesKey, entry.first);
            RedisMgr::getInstance().hDel(constants::redis::kLoginCountKey, entry.first);
            ++cleaned;
            continue;
        }

        if (!isAlive(node))
        {
            RedisMgr::getInstance().hDel(constants::redis::kRegisteredNodesKey, entry.first);
            RedisMgr::getInstance().hDel(constants::redis::kLoginCountKey, entry.first);
            ++cleaned;
            Log::info(LogModule::Registry,
                      "cleanupExpiredNodes: removed expired node {} (expired at {})",
                      entry.first, node.expire_at);
        }
        else
        {
            ++alive;
        }
    }

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Registry,
              "cleanupExpiredNodes: scanned={} alive={} cleaned={} cost={}ms",
              static_cast<int>(all.size()), alive, cleaned, cost_ms);
}

// 注册节点：先清理过期节点，若同名节点已存活且 instance_id 不同则拒绝
bool RedisNodeRegistryImpl::registerNode(const NodeInfo &node)
{
    const auto start = std::chrono::steady_clock::now();
    if (node.name.empty())
    {
        Log::warn(LogModule::Registry, "registerNode: node name is empty");
        return false;
    }

    cleanupExpiredNodes();

    auto existing = getNode(node.name);
    if (existing && isAlive(*existing) && existing->instance_id != node.instance_id)
    {
        Log::warn(LogModule::Registry,
                  "registerNode: node {} already registered by another instance {} (current {})",
                  node.name, existing->instance_id, node.instance_id);
        return false;
    }

    NodeInfo stored = node;
    stored.expire_at = utils::time::nowSec() + constants::business::kNodeTtlSeconds;

    auto &redis = RedisMgr::getInstance();
    if (!redis.hSet(constants::redis::kRegisteredNodesKey, node.name, serializeNode(stored)))
    {
        Log::error(LogModule::Registry, "registerNode: failed to save node {} to Redis", node.name);
        return false;
    }

    redis.hSet(constants::redis::kLoginCountKey, node.name, "0");

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Registry, "registerNode: registered node {} instance {} cost={}ms",
              node.name, node.instance_id, cost_ms);
    return true;
}

// 注销节点：清理节点绑定的所有用户路由，再删除节点记录
bool RedisNodeRegistryImpl::unregisterNode(const std::string &name, const std::string &instance_id)
{
    const auto start = std::chrono::steady_clock::now();
    auto existing = getNode(name);
    if (!existing)
    {
        Log::info(LogModule::Registry, "unregisterNode: node {} not found, nothing to do", name);
        return true;
    }

    if (!instance_id.empty() && existing->instance_id != instance_id)
    {
        Log::warn(LogModule::Registry,
                  "unregisterNode: instance mismatch for node {} (expected {}, got {})", name,
                  existing->instance_id, instance_id);
        return false;
    }

    auto &redis = RedisMgr::getInstance();

    // 清理该节点绑定的所有用户
    const std::string usersKey = std::string(constants::redis::kNodeUsersPrefix) + name;
    std::vector<std::string> uids;
    redis.sMembers(usersKey, uids);
    Log::info(LogModule::Registry, "unregisterNode: removing node {}, clearing {} bound user(s)",
              name, uids.size());
    for (const auto &uidStr : uids)
    {
        redis.del(std::string(constants::redis::kUserNodePrefix) + uidStr);
    }

    redis.del(usersKey);
    redis.hDel(constants::redis::kRegisteredNodesKey, name);
    redis.hDel(constants::redis::kLoginCountKey, name);

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Registry, "unregisterNode: unregistered node {} cost={}ms", name,
              cost_ms);
    return true;
}

// 心跳续期：刷新节点的 expire_at 为当前时间 + TTL
bool RedisNodeRegistryImpl::heartbeat(const std::string &name, const std::string &instance_id)
{
    const auto start = std::chrono::steady_clock::now();
    auto existing = getNode(name);
    if (!existing || existing->instance_id != instance_id)
    {
        Log::warn(LogModule::Registry,
                  "heartbeat: node {} not found or instance mismatch (expected {}, got {})", name,
                  existing ? existing->instance_id : "N/A", instance_id);
        return false;
    }

    NodeInfo updated = *existing;
    updated.expire_at = utils::time::nowSec() + constants::business::kNodeTtlSeconds;

    bool ok = RedisMgr::getInstance().hSet(constants::redis::kRegisteredNodesKey, name, serializeNode(updated));
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    if (ok)
    {
        Log::debug(LogModule::Registry, "heartbeat: renewed node {} instance {} cost={}ms", name,
                   instance_id, cost_ms);
    }
    else
    {
        Log::error(LogModule::Registry, "heartbeat: failed to update node {} in Redis", name);
    }
    return ok;
}

// 按名称查询节点，若不存在或已过期返回 nullopt
std::optional<NodeInfo> RedisNodeRegistryImpl::getNode(const std::string &name)
{
    const std::string json = RedisMgr::getInstance().hGet(constants::redis::kRegisteredNodesKey, name);
    if (json.empty())
    {
        Log::debug(LogModule::Registry, "getNode: node {} not found in Redis", name);
        return std::nullopt;
    }

    NodeInfo node;
    node.name = name;
    if (!parseNode(json, node) || !isAlive(node))
    {
        Log::debug(LogModule::Registry, "getNode: node {} is expired or invalid", name);
        return std::nullopt;
    }
    return node;
}

// 列出 Redis 中所有存活的节点
std::vector<NodeInfo> RedisNodeRegistryImpl::listNodes()
{
    std::vector<NodeInfo> result;
    std::map<std::string, std::string> all;

    if (!RedisMgr::getInstance().hGetAll(constants::redis::kRegisteredNodesKey, all))
    {
        Log::warn(LogModule::Registry, "listNodes: failed to get all nodes from Redis");
        return result;
    }

    for (const auto &entry : all)
    {
        NodeInfo node;
        node.name = entry.first;
        if (parseNode(entry.second, node) && isAlive(node))
        {
            result.push_back(std::move(node));
        }
    }

    Log::info(LogModule::Registry, "listNodes: found {} alive node(s)", result.size());
    return result;
}

// 查询用户当前绑定的节点
std::optional<NodeInfo> RedisNodeRegistryImpl::getNodeForUser(int uid)
{
    std::string nodeName;
    if (!RedisMgr::getInstance().get(std::string(constants::redis::kUserNodePrefix) + std::to_string(uid),
                                     nodeName) ||
        nodeName.empty())
    {
        Log::debug(LogModule::Registry, "getNodeForUser: user {} not bound to any node", uid);
        return std::nullopt;
    }

    auto node = getNode(nodeName);
    if (node)
    {
        Log::debug(LogModule::Registry, "getNodeForUser: user {} -> node {}", uid, nodeName);
    }
    else
    {
        Log::warn(LogModule::Registry,
                  "getNodeForUser: user {} bound to node {} but node is unavailable", uid,
                  nodeName);
    }
    return node;
}

// 将用户绑定到指定节点，若用户之前绑定在其他节点则先解绑旧关系
bool RedisNodeRegistryImpl::bindUser(int uid, const std::string &node_name)
{
    if (uid <= 0 || node_name.empty())
    {
        Log::warn(LogModule::Registry, "bindUser: invalid params uid={} node_name='{}'", uid,
                  node_name);
        return false;
    }

    if (!getNode(node_name))
    {
        Log::warn(LogModule::Registry, "bindUser: target node {} is not alive for user {}",
                  node_name, uid);
        return false;
    }

    const std::string uidStr = std::to_string(uid);
    auto &redis = RedisMgr::getInstance();

    // 检查用户是否已绑定到其他节点，若是则解绑旧关系
    std::string oldNode;
    if (redis.get(std::string(constants::redis::kUserNodePrefix) + uidStr, oldNode) && !oldNode.empty() &&
        oldNode != node_name)
    {
        redis.sRem(std::string(constants::redis::kNodeUsersPrefix) + oldNode, uidStr);
        Log::info(LogModule::Registry, "bindUser: moved user {} from old node {} to {}", uid,
                  oldNode, node_name);
    }

    redis.set(std::string(constants::redis::kUserNodePrefix) + uidStr, node_name);
    redis.sAdd(std::string(constants::redis::kNodeUsersPrefix) + node_name, uidStr);

    // 原子增加该节点的登录计数
    auto newCount = RedisMgr::getInstance().hIncrBy(constants::redis::kLoginCountKey, node_name, 1);
    Log::info(LogModule::Registry, "bindUser: node {} login count incremented to {}", node_name,
              newCount);

    Log::info(LogModule::Registry, "bindUser: user {} bound to node {}", uid, node_name);
    return true;
}

// 解绑用户与节点的绑定关系
bool RedisNodeRegistryImpl::unbindUser(int uid)
{
    if (uid <= 0)
    {
        Log::warn(LogModule::Registry, "unbindUser: invalid uid {}", uid);
        return false;
    }

    const std::string uidStr = std::to_string(uid);
    auto &redis = RedisMgr::getInstance();

    std::string nodeName;
    if (redis.get(std::string(constants::redis::kUserNodePrefix) + uidStr, nodeName) && !nodeName.empty())
    {
        redis.sRem(std::string(constants::redis::kNodeUsersPrefix) + nodeName, uidStr);
        Log::info(LogModule::Registry, "unbindUser: removed user {} from node {}", uid, nodeName);
    }

    redis.del(std::string(constants::redis::kUserNodePrefix) + uidStr);

    // 原子减少该节点的登录计数
    if (!nodeName.empty())
    {
        auto newCount = RedisMgr::getInstance().hIncrBy(constants::redis::kLoginCountKey, nodeName, -1);
        if (newCount < 0)
        {
            RedisMgr::getInstance().hSet(constants::redis::kLoginCountKey, nodeName, "0");
            newCount = 0;
        }
        Log::info(LogModule::Registry, "unbindUser: node {} login count updated to {}", nodeName,
                  newCount);
    }

    Log::info(LogModule::Registry, "unbindUser: user {} unbound", uid);
    return true;
}

// 刷新登录 token TTL
bool RedisNodeRegistryImpl::refreshTokenTTL(int uid)
{
    if (uid <= 0)
    {
        Log::warn(LogModule::Registry, "refreshTokenTTL: invalid uid {}", uid);
        return false;
    }

    // 通过 uid -> token 查找索引定位主 key
    std::string uidStr = std::to_string(uid);
    std::string lookupKey = std::string(constants::redis::kUserTokenLookupPrefix) + uidStr;
    std::string token;
    if (!RedisMgr::getInstance().get(lookupKey, token) || token.empty())
    {
        Log::warn(LogModule::Registry, "refreshTokenTTL: no token for uid={}", uid);
        return false;
    }

    std::string tokenKey = std::string(constants::redis::kUserTokenPrefix) + token;
    auto &redis = RedisMgr::getInstance();
    bool ok1 = redis.expire(tokenKey, constants::business::kTokenTtlSeconds);
    bool ok2 = redis.expire(lookupKey, constants::business::kTokenTtlSeconds);

    Log::debug(LogModule::Registry,
               "refreshTokenTTL: uid={} tokenKey={} ok1={} ok2={} ttl={}s", uid, tokenKey, ok1,
               ok2, constants::business::kTokenTtlSeconds);
    return ok1 && ok2;
}

// 删除登录 token
bool RedisNodeRegistryImpl::deleteToken(int uid)
{
    if (uid <= 0)
    {
        Log::warn(LogModule::Registry, "deleteToken: invalid uid {}", uid);
        return false;
    }

    // 通过 uid -> token 查找索引定位主 key
    std::string uidStr = std::to_string(uid);
    std::string lookupKey = std::string(constants::redis::kUserTokenLookupPrefix) + uidStr;
    std::string token;
    if (!RedisMgr::getInstance().get(lookupKey, token))
    {
        Log::warn(LogModule::Registry, "deleteToken: no token for uid={}", uid);
        return false;
    }

    auto &redis = RedisMgr::getInstance();
    bool ok = redis.del(lookupKey);
    if (!token.empty())
    {
        std::string tokenKey = std::string(constants::redis::kUserTokenPrefix) + token;
        redis.del(tokenKey);
    }

    Log::debug(LogModule::Registry, "deleteToken: uid={} token={} ok={}", uid, token, ok);
    return ok;
}

// 统一清理用户登录数据：解绑节点 + 删除 token
bool RedisNodeRegistryImpl::clearUserLoginData(int uid)
{
    if (uid <= 0)
    {
        Log::warn(LogModule::Registry, "clearUserLoginData: invalid uid {}", uid);
        return false;
    }

    const auto start = std::chrono::steady_clock::now();
    bool ok = true;

    if (!unbindUser(uid))
    {
        Log::warn(LogModule::Registry, "clearUserLoginData: unbindUser failed uid={}", uid);
        ok = false;
    }

    if (!deleteToken(uid))
    {
        Log::warn(LogModule::Registry, "clearUserLoginData: deleteToken failed uid={}", uid);
        ok = false;
    }

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    Log::info(LogModule::Registry, "clearUserLoginData: uid={} ok={} cost={}ms", uid, ok,
              cost_ms);
    return ok;
}

// 选取当前登录用户数最少的节点（简单负载均衡）
std::optional<NodeInfo> RedisNodeRegistryImpl::selectLeastLoadedNode()
{
    const auto start = std::chrono::steady_clock::now();
    auto nodes = listNodes();
    if (nodes.empty())
    {
        Log::warn(LogModule::Registry, "selectLeastLoadedNode: no alive nodes available");
        return std::nullopt;
    }

    NodeInfo best;
    int bestCount = INT_MAX;
    std::string load_details;

    for (const auto &node : nodes)
    {
        if (node.name.find("Node-") != 0)
        {
            Log::debug(LogModule::Registry,
                       "selectLeastLoadedNode: skip non-ChatServer node {}", node.name);
            continue;
        }

        auto countStr = RedisMgr::getInstance().hGet(constants::redis::kLoginCountKey, node.name);
        int count = countStr.empty() ? INT_MAX : std::stoi(countStr);
        if (!load_details.empty())
        {
            load_details += ", ";
        }
        load_details += node.name + "=" + std::to_string(count);

        if (count < bestCount)
        {
            bestCount = count;
            best = node;
        }
    }

    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    if (best.name.empty())
    {
        Log::warn(LogModule::Registry, "selectLeastLoadedNode: no ChatServer node available");
        return std::nullopt;
    }

    Log::info(LogModule::Registry,
              "selectLeastLoadedNode: selected node {} with {} login(s) candidates=[{}] cost={}ms",
              best.name, bestCount, load_details, cost_ms);
    return best;
}
