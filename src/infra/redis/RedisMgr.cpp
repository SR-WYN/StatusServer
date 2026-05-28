#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "RedisConPool.h"
#include "RedisSession.h"

RedisMgr::RedisMgr()
{
    auto &cfg = ConfigMgr::getInstance();
    auto host = cfg["Redis"]["Host"];
    auto port = cfg["Redis"]["Port"];
    auto pwd = cfg["Redis"]["Passwd"];
    _con_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr() = default;

bool RedisMgr::connect(const std::string &host, int port)
{
    (void)host;
    (void)port;
    return RedisSession::withConn(*_con_pool, [](redisContext *ctx) { return !ctx->err; });
}

bool RedisMgr::get(const std::string &key, std::string &value)
{
    return RedisSession::get(*_con_pool, key, value);
}

bool RedisMgr::set(const std::string &key, const std::string &value)
{
    return RedisSession::set(*_con_pool, key, value);
}

bool RedisMgr::auth(const std::string &password)
{
    return RedisSession::auth(*_con_pool, password);
}

bool RedisMgr::lPush(const std::string &key, const std::string &value)
{
    return RedisSession::lPush(*_con_pool, key, value);
}

bool RedisMgr::lPop(const std::string &key, std::string &value)
{
    return RedisSession::lPop(*_con_pool, key, value);
}

bool RedisMgr::rPush(const std::string &key, const std::string &value)
{
    return RedisSession::rPush(*_con_pool, key, value);
}

bool RedisMgr::rPop(const std::string &key, std::string &value)
{
    return RedisSession::rPop(*_con_pool, key, value);
}

bool RedisMgr::hSet(const std::string &key, const std::string &field, const std::string &value)
{
    return RedisSession::hSet(*_con_pool, key, field, value);
}

bool RedisMgr::hSet(const char *key, const char *field, const char *val, size_t vallen)
{
    return RedisSession::hSet(*_con_pool, std::string(key), std::string(field),
                             std::string(val, vallen));
}

std::string RedisMgr::hGet(const std::string &key, const std::string &field)
{
    return RedisSession::hGet(*_con_pool, key, field);
}

bool RedisMgr::hDel(const std::string &key, const std::string &field)
{
    return RedisSession::hDel(*_con_pool, key, field);
}

bool RedisMgr::hGetAll(const std::string &key, std::map<std::string, std::string> &out)
{
    return RedisSession::hGetAll(*_con_pool, key, out);
}

bool RedisMgr::sAdd(const std::string &key, const std::string &member)
{
    return RedisSession::sAdd(*_con_pool, key, member);
}

bool RedisMgr::sRem(const std::string &key, const std::string &member)
{
    return RedisSession::sRem(*_con_pool, key, member);
}

bool RedisMgr::sMembers(const std::string &key, std::vector<std::string> &members)
{
    return RedisSession::sMembers(*_con_pool, key, members);
}

bool RedisMgr::del(const std::string &key)
{
    return RedisSession::del(*_con_pool, key);
}

bool RedisMgr::existsKey(const std::string &key)
{
    return RedisSession::existsKey(*_con_pool, key);
}

void RedisMgr::close()
{
    _con_pool->close();
}
