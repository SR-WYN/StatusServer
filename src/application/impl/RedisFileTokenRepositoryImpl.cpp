// RedisFileTokenRepositoryImpl.cpp — Redis 实现的文件传输临时 token 存储
#include "RedisFileTokenRepositoryImpl.h"
#include "RedisMgr.h"
#include "const.h"
#include "Log.h"

bool RedisFileTokenRepositoryImpl::saveFileToken(int uid, const std::string& token, int ttl_sec)
{
    std::string key = std::string(RedisPrefix::FILETOKENPREFIX) + std::to_string(uid);
    bool ok = RedisMgr::getInstance().setEx(key, token, ttl_sec);
    if (ok)
    {
        Log::info(LogModule::Registry, "saveFileToken: uid={} token={} ttl={}s key={}", uid,
                  token, ttl_sec, key);
    }
    else
    {
        Log::warn(LogModule::Registry, "saveFileToken failed: uid={} key={}", uid, key);
    }
    return ok;
}

bool RedisFileTokenRepositoryImpl::deleteFileToken(int uid)
{
    std::string key = std::string(RedisPrefix::FILETOKENPREFIX) + std::to_string(uid);
    int del = RedisMgr::getInstance().del(key);
    if (del < 0)
    {
        Log::warn(LogModule::Registry, "deleteFileToken failed: uid={} key={} del={}", uid, key,
                  del);
        return false;
    }
    Log::info(LogModule::Registry, "deleteFileToken: uid={} key={} del={}", uid, key, del);
    return true;
}
