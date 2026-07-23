// MemoryNodeRegistryImpl.h - 内存版 NodeRegistry，用于测试 Token 校验与负载均衡行为
#pragma once

#include "NodeRegistry.h"
#include "error_codes.h"

#include <climits>
#include <map>
#include <set>
#include <string>

/// 内存版 NodeRegistry 实现
/// 仅用于单元测试，用 STL 容器替代 Redis，验证 NodeRegistry 接口契约、
/// Token 校验逻辑与负载均衡算法。
class MemoryNodeRegistryImpl : public NodeRegistry
{
public:
    bool registerNode(const NodeInfo &node) override
    {
        if (node.name.empty())
            return false;

        nodes_[node.name] = node;
        if (login_count_.find(node.name) == login_count_.end())
            login_count_[node.name] = 0;
        return true;
    }

    bool unregisterNode(const std::string &name, const std::string &) override
    {
        nodes_.erase(name);
        login_count_.erase(name);
        node_users_.erase(name);
        return true;
    }

    bool heartbeat(const std::string &, const std::string &) override { return true; }

    std::optional<NodeInfo> getNode(const std::string &name) override
    {
        auto it = nodes_.find(name);
        if (it != nodes_.end())
            return it->second;
        return std::nullopt;
    }

    std::optional<NodeInfo> getNodeForUser(int uid) override
    {
        auto it = user_node_.find(uid);
        if (it == user_node_.end())
            return std::nullopt;
        return getNode(it->second);
    }

    std::vector<NodeInfo> listNodes() override
    {
        std::vector<NodeInfo> result;
        for (const auto &entry : nodes_)
            result.push_back(entry.second);
        return result;
    }

    bool bindUser(int uid, const std::string &node_name) override
    {
        if (uid <= 0 || nodes_.find(node_name) == nodes_.end())
            return false;

        // 若已绑定到其他节点，先解绑旧节点
        auto old = user_node_.find(uid);
        if (old != user_node_.end() && old->second != node_name)
        {
            const std::string old_node = old->second;
            node_users_[old_node].erase(std::to_string(uid));
            if (login_count_[old_node] > 0)
                login_count_[old_node]--;
        }

        user_node_[uid] = node_name;
        node_users_[node_name].insert(std::to_string(uid));
        login_count_[node_name] = static_cast<int>(node_users_[node_name].size());
        return true;
    }

    bool unbindUser(int uid) override
    {
        auto it = user_node_.find(uid);
        if (it == user_node_.end())
            return false;

        const std::string node_name = it->second;
        user_node_.erase(it);
        node_users_[node_name].erase(std::to_string(uid));
        login_count_[node_name] = static_cast<int>(node_users_[node_name].size());
        return true;
    }

    bool refreshTokenTTL(int) override { return true; }

    bool deleteToken(int uid) override
    {
        auto it = user_tokens_.find(uid);
        if (it != user_tokens_.end())
        {
            token_to_uid_.erase(it->second);
            user_tokens_.erase(it);
        }
        return true;
    }

    bool clearUserLoginData(int uid) override
    {
        unbindUser(uid);
        deleteToken(uid);
        return true;
    }

    std::optional<NodeInfo> selectLeastLoadedNode() override
    {
        std::optional<NodeInfo> best;
        int best_count = INT_MAX;

        for (const auto &entry : nodes_)
        {
            // 只考虑 ChatServer 节点（名称以 "Node-" 开头）
            if (entry.first.find("Node-") != 0)
                continue;

            int count = login_count_[entry.first];
            if (count < best_count)
            {
                best_count = count;
                best = entry.second;
            }
        }
        return best;
    }

    void cleanupExpiredNodes() override {}

    bool saveToken(int uid, const std::string &token) override
    {
        if (uid <= 0)
            return false;
        user_tokens_[uid] = token;
        token_to_uid_[token] = uid;
        return true;
    }

    int resolveToken(const std::string &token) override
    {
        auto it = token_to_uid_.find(token);
        if (it != token_to_uid_.end())
            return it->second;
        return 0;
    }

    // 测试辅助：保存文件传输 token
    bool saveFileToken(int uid, const std::string &token)
    {
        if (uid <= 0)
            return false;
        file_tokens_[uid] = token;
        return true;
    }

    int validateToken(int uid, const std::string &token) override
    {
        if (uid <= 0)
            return ErrorCodes::UID_INVALID;

        auto uit = user_tokens_.find(uid);
        if (uit != user_tokens_.end() && uit->second == token)
            return ErrorCodes::SUCCESS;

        auto fit = file_tokens_.find(uid);
        if (fit != file_tokens_.end() && fit->second == token)
            return ErrorCodes::SUCCESS;

        return ErrorCodes::TOKEN_INVALID;
    }

private:
    std::map<std::string, NodeInfo> nodes_;
    std::map<int, std::string> user_node_;
    std::map<std::string, std::set<std::string>> node_users_;
    std::map<std::string, int> login_count_;
    std::map<int, std::string> user_tokens_;
    std::map<std::string, int> token_to_uid_;
    std::map<int, std::string> file_tokens_;
};
