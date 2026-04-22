#pragma once
#include "Singleton.h"
#include <hiredis/hiredis.h>
#include <string>
#include <memory>

class RedisConPool;

class RedisMgr : public Singleton<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    ~RedisMgr();
    bool connect(const std::string& host, int port);
    bool get(const std::string& key, std::string& value);
    bool set(const std::string& key, const std::string& value);
    bool auth(const std::string& password);
    bool lPush(const std::string& key, const std::string& value);
    bool lPop(const std::string& key, std::string& value);
    bool rPush(const std::string& key, const std::string& value);
    bool rPop(const std::string& key, std::string& value);
    bool hSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool hSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string hGet(const std::string& key, const std::string& hkey);
    bool del(const std::string& key);
    bool existsKey(const std::string& key);
    void close();

private:
    RedisMgr();
    std::unique_ptr<RedisConPool> _con_pool;
};