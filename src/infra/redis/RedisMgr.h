#pragma once
#include "Singleton.h"
#include <map>
#include <memory>
#include <string>
#include <sw/redis++/redis++.h>
#include <vector>

// redis-plus-plus 封装，对外保持与原有 hiredis 版本一致的接口
class RedisMgr : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    ~RedisMgr();

    // 基础 KV 操作
    bool get(const std::string &key, std::string &value);
    bool set(const std::string &key, const std::string &value);
    bool del(const std::string &key);
    bool existsKey(const std::string &key);

    // Hash 操作
    bool hSet(const std::string &key, const std::string &field, const std::string &value);
    bool hSet(const char *key, const char *field, const char *val, size_t vallen);
    std::string hGet(const std::string &key, const std::string &field);
    bool hDel(const std::string &key, const std::string &field);
    bool hGetAll(const std::string &key, std::map<std::string, std::string> &out);

    // List 操作
    bool lPush(const std::string &key, const std::string &value);
    bool lPop(const std::string &key, std::string &value);
    bool rPush(const std::string &key, const std::string &value);
    bool rPop(const std::string &key, std::string &value);

    // Set 操作
    bool sAdd(const std::string &key, const std::string &member);
    bool sRem(const std::string &key, const std::string &member);
    bool sMembers(const std::string &key, std::vector<std::string> &members);

    void close();

private:
    RedisMgr();
    std::unique_ptr<sw::redis::Redis> _redis; // redis-plus-plus 客户端
};
