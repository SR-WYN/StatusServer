#include "RedisMgr.h"
#include "ConfigMgr.h"

RedisMgr::RedisMgr()
{
    auto &cfg = ConfigMgr::getInstance();
    auto host = cfg["Redis"]["Host"];
    auto port = cfg["Redis"]["Port"];
    auto pwd = cfg["Redis"]["Passwd"];

    // 配置连接池（大小 5）和连接参数
    sw::redis::ConnectionPoolOptions pool_opts;
    pool_opts.size = 5;
    sw::redis::ConnectionOptions conn_opts;
    conn_opts.host = host;
    conn_opts.port = std::stoi(port);
    conn_opts.password = pwd;
    conn_opts.socket_timeout = std::chrono::milliseconds(3000);

    _redis = std::make_unique<sw::redis::Redis>(conn_opts, pool_opts);
}

RedisMgr::~RedisMgr() = default;

bool RedisMgr::get(const std::string &key, std::string &value)
{
    try
    {
        auto val = _redis->get(key);
        if (val)
        {
            value = *val;
            return true;
        }
        return false;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::set(const std::string &key, const std::string &value)
{
    try
    {
        _redis->set(key, value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::del(const std::string &key)
{
    try
    {
        return _redis->del(key) > 0;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::existsKey(const std::string &key)
{
    try
    {
        return _redis->exists(key) > 0;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::hSet(const std::string &key, const std::string &field, const std::string &value)
{
    try
    {
        _redis->hset(key, field, value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::hSet(const char *key, const char *field, const char *val, size_t vallen)
{
    return hSet(std::string(key), std::string(field), std::string(val, vallen));
}

std::string RedisMgr::hGet(const std::string &key, const std::string &field)
{
    try
    {
        auto val = _redis->hget(key, field);
        return val ? *val : "";
    }
    catch (...)
    {
        return "";
    }
}

bool RedisMgr::hDel(const std::string &key, const std::string &field)
{
    try
    {
        return _redis->hdel(key, field) > 0;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::hGetAll(const std::string &key, std::map<std::string, std::string> &out)
{
    try
    {
        out.clear();
        _redis->hgetall(key, std::inserter(out, out.end()));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::lPush(const std::string &key, const std::string &value)
{
    try
    {
        return _redis->lpush(key, value) > 0;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::lPop(const std::string &key, std::string &value)
{
    try
    {
        auto val = _redis->lpop(key);
        if (val)
        {
            value = *val;
            return true;
        }
        return false;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::rPush(const std::string &key, const std::string &value)
{
    try
    {
        return _redis->rpush(key, value) > 0;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::rPop(const std::string &key, std::string &value)
{
    try
    {
        auto val = _redis->rpop(key);
        if (val)
        {
            value = *val;
            return true;
        }
        return false;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::sAdd(const std::string &key, const std::string &member)
{
    try
    {
        return _redis->sadd(key, member) > 0;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::sRem(const std::string &key, const std::string &member)
{
    try
    {
        return _redis->srem(key, member) > 0;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::sMembers(const std::string &key, std::vector<std::string> &members)
{
    try
    {
        members.clear();
        _redis->smembers(key, std::back_inserter(members));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void RedisMgr::close()
{
    // redis-plus-plus 析构时自动释放连接池
    _redis.reset();
}
