// RedisMgr.cpp - Redis 客户端封装实现
#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "Log.h"

#include <chrono>
#include <sstream>

namespace
{
// Redis 操作日志守卫：记录入口、出口耗时，对慢操作打 warn
class RedisLogGuard
{
public:
    RedisLogGuard(const std::string &cmd, const std::string &key)
        : _cmd(cmd), _key(key), _start(std::chrono::steady_clock::now())
    {
        Log::trace(LogModule::Redis, "Redis {} {} start", _cmd, _key);
    }

    RedisLogGuard(const std::string &cmd, const std::string &key, const std::string &field)
        : _cmd(cmd), _key(key + ":" + field), _start(std::chrono::steady_clock::now())
    {
        Log::trace(LogModule::Redis, "Redis {} {} start", _cmd, _key);
    }

    ~RedisLogGuard()
    {
        const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - _start)
                                 .count();
        if (cost_ms > 100)
        {
            Log::warn(LogModule::Redis, "Redis {} {} slow cost={}ms", _cmd, _key, cost_ms);
        }
        else
        {
            Log::trace(LogModule::Redis, "Redis {} {} end cost={}ms", _cmd, _key, cost_ms);
        }
    }

private:
    std::string _cmd;
    std::string _key;
    std::chrono::steady_clock::time_point _start;
};
} // namespace

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
    Log::info(LogModule::Redis, "connected to Redis at {}:{} (pool size={}, pwd_set={})", host,
              port, pool_opts.size, !pwd.empty());
}

RedisMgr::~RedisMgr()
{
    Log::info(LogModule::Redis, "RedisMgr destroyed");
}

bool RedisMgr::get(const std::string &key, std::string &value)
{
    RedisLogGuard guard("GET", key);
    try
    {
        auto val = _redis->get(key);
        if (val)
        {
            value = *val;
            Log::debug(LogModule::Redis, "GET {} hit", key);
            return true;
        }
        Log::debug(LogModule::Redis, "GET {} miss", key);
        return false;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "GET {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::set(const std::string &key, const std::string &value)
{
    RedisLogGuard guard("SET", key);
    try
    {
        _redis->set(key, value);
        Log::debug(LogModule::Redis, "SET {} ok", key);
        return true;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "SET {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::setEx(const std::string &key, const std::string &value, int ttl_sec)
{
    RedisLogGuard guard("SETEX", key);
    try
    {
        _redis->set(key, value, std::chrono::seconds(ttl_sec));
        Log::debug(LogModule::Redis, "SETEX {} ttl={}s ok", key, ttl_sec);
        return true;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "SETEX {} {}s failed: {}", key, ttl_sec, e.what());
        return false;
    }
}

bool RedisMgr::del(const std::string &key)
{
    RedisLogGuard guard("DEL", key);
    try
    {
        auto deleted = _redis->del(key);
        Log::debug(LogModule::Redis, "DEL {} deleted={}", key, deleted);
        return deleted > 0;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "DEL {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::existsKey(const std::string &key)
{
    RedisLogGuard guard("EXISTS", key);
    try
    {
        auto exists = _redis->exists(key) > 0;
        Log::debug(LogModule::Redis, "EXISTS {} result={}", key, exists);
        return exists;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "EXISTS {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::expire(const std::string &key, int ttl_seconds)
{
    RedisLogGuard guard("EXPIRE", key);
    try
    {
        auto ok = _redis->expire(key, std::chrono::seconds(ttl_seconds));
        Log::debug(LogModule::Redis, "EXPIRE {} ttl={}s ok={}", key, ttl_seconds, ok);
        return ok;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "EXPIRE {} {}s failed: {}", key, ttl_seconds, e.what());
        return false;
    }
}

bool RedisMgr::hSet(const std::string &key, const std::string &field, const std::string &value)
{
    RedisLogGuard guard("HSET", key, field);
    try
    {
        _redis->hset(key, field, value);
        Log::debug(LogModule::Redis, "HSET {}:{} ok", key, field);
        return true;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "HSET {} {} failed: {}", key, field, e.what());
        return false;
    }
}

bool RedisMgr::hSet(const char *key, const char *field, const char *val, size_t vallen)
{
    return hSet(std::string(key), std::string(field), std::string(val, vallen));
}

std::string RedisMgr::hGet(const std::string &key, const std::string &field)
{
    RedisLogGuard guard("HGET", key, field);
    try
    {
        auto val = _redis->hget(key, field);
        bool hit = val.has_value();
        Log::debug(LogModule::Redis, "HGET {}:{} hit={}", key, field, hit);
        return val ? *val : "";
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "HGET {} {} failed: {}", key, field, e.what());
        return "";
    }
}

bool RedisMgr::hDel(const std::string &key, const std::string &field)
{
    RedisLogGuard guard("HDEL", key, field);
    try
    {
        auto deleted = _redis->hdel(key, field);
        Log::debug(LogModule::Redis, "HDEL {}:{} deleted={}", key, field, deleted);
        return deleted > 0;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "HDEL {} {} failed: {}", key, field, e.what());
        return false;
    }
}

bool RedisMgr::hGetAll(const std::string &key, std::map<std::string, std::string> &out)
{
    RedisLogGuard guard("HGETALL", key);
    try
    {
        out.clear();
        _redis->hgetall(key, std::inserter(out, out.end()));
        Log::debug(LogModule::Redis, "HGETALL {} entries={}", key, out.size());
        return true;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "HGETALL {} failed: {}", key, e.what());
        return false;
    }
}

long long RedisMgr::hIncrBy(const std::string &key, const std::string &field, long long increment)
{
    RedisLogGuard guard("HINCRBY", key, field);
    try
    {
        auto result = _redis->hincrby(key, field, increment);
        Log::debug(LogModule::Redis, "HINCRBY {}:{} by={} -> {}", key, field, increment, result);
        return result;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "HINCRBY {} {} {} failed: {}", key, field, increment,
                   e.what());
        return 0;
    }
}

bool RedisMgr::lPush(const std::string &key, const std::string &value)
{
    RedisLogGuard guard("LPUSH", key);
    try
    {
        auto len = _redis->lpush(key, value);
        Log::debug(LogModule::Redis, "LPUSH {} len={}", key, len);
        return len > 0;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "LPUSH {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::lPop(const std::string &key, std::string &value)
{
    RedisLogGuard guard("LPOP", key);
    try
    {
        auto val = _redis->lpop(key);
        if (val)
        {
            value = *val;
            Log::debug(LogModule::Redis, "LPOP {} hit", key);
            return true;
        }
        Log::debug(LogModule::Redis, "LPOP {} miss", key);
        return false;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "LPOP {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::rPush(const std::string &key, const std::string &value)
{
    RedisLogGuard guard("RPUSH", key);
    try
    {
        auto len = _redis->rpush(key, value);
        Log::debug(LogModule::Redis, "RPUSH {} len={}", key, len);
        return len > 0;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "RPUSH {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::rPop(const std::string &key, std::string &value)
{
    RedisLogGuard guard("RPOP", key);
    try
    {
        auto val = _redis->rpop(key);
        if (val)
        {
            value = *val;
            Log::debug(LogModule::Redis, "RPOP {} hit", key);
            return true;
        }
        Log::debug(LogModule::Redis, "RPOP {} miss", key);
        return false;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "RPOP {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::sAdd(const std::string &key, const std::string &member)
{
    RedisLogGuard guard("SADD", key);
    try
    {
        auto added = _redis->sadd(key, member);
        Log::debug(LogModule::Redis, "SADD {} member={} added={}", key, member, added);
        return added > 0;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "SADD {} {} failed: {}", key, member, e.what());
        return false;
    }
}

bool RedisMgr::sRem(const std::string &key, const std::string &member)
{
    RedisLogGuard guard("SREM", key);
    try
    {
        auto removed = _redis->srem(key, member);
        Log::debug(LogModule::Redis, "SREM {} member={} removed={}", key, member, removed);
        return removed > 0;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "SREM {} {} failed: {}", key, member, e.what());
        return false;
    }
}

bool RedisMgr::sMembers(const std::string &key, std::vector<std::string> &members)
{
    RedisLogGuard guard("SMEMBERS", key);
    try
    {
        members.clear();
        _redis->smembers(key, std::back_inserter(members));
        Log::debug(LogModule::Redis, "SMEMBERS {} count={}", key, members.size());
        return true;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "SMEMBERS {} failed: {}", key, e.what());
        return false;
    }
}

bool RedisMgr::publish(const std::string &channel, const std::string &message)
{
    RedisLogGuard guard("PUBLISH", channel);
    try
    {
        auto receivers = _redis->publish(channel, message);
        Log::debug(LogModule::Redis, "PUBLISH {} receivers={}", channel, receivers);
        return true;
    }
    catch (const sw::redis::Error &e)
    {
        Log::error(LogModule::Redis, "PUBLISH {} failed: {}", channel, e.what());
        return false;
    }
}

void RedisMgr::close()
{
    Log::info(LogModule::Redis, "RedisMgr closing");
    _redis.reset();
}
