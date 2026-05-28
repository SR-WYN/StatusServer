#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct RegisteredChatNode
{
    std::string name;
    std::string client_host;
    std::string client_port;
    std::string rpc_host;
    std::string rpc_port;
    std::string instance_id;
    int64_t expire_at = 0;
};

class ChatNodeRegistry
{
public:
    static bool registerNode(const RegisteredChatNode &node);
    static bool unregisterNode(const std::string &name, const std::string &instance_id);
    static bool heartbeat(const std::string &name, const std::string &instance_id);
    static std::optional<RegisteredChatNode> getNode(const std::string &name);
    static std::optional<RegisteredChatNode> getUserNode(int uid);
    static std::vector<RegisteredChatNode> listAliveNodes();
    static bool bindUser(int uid, const std::string &node_name);
    static bool unbindUser(int uid);
    static std::optional<RegisteredChatNode> pickLeastLoadedNode();
    /** 删除 chat_nodes 中已过期的条目（进程崩溃未 Unregister 时残留） */
    static void purgeExpiredNodes();

private:
    static std::string serializeNode(const RegisteredChatNode &node);
    static bool parseNode(const std::string &json, RegisteredChatNode &out);
    static bool isAlive(const RegisteredChatNode &node);
    static int64_t nowSec();
};
