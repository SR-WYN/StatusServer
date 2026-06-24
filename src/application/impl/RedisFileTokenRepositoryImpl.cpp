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
        Log::info(LogModule::Registry,
                  "saveFileToken: uid={} ttl={}s", uid, ttl_sec);
    }
    else
    {
        Log::warn(LogModule::Registry,
                  "saveFileToken failed: uid={}", uid);
    }
    return ok;
}

bool RedisFileTokenRepositoryImpl::deleteFileToken(int uid)
{
    std::string key = std::string(RedisPrefix::FILETOKENPREFIX) + std::to_string(uid);
    int del = RedisMgr::getInstance().del(key);
    Log::info(LogModule::Registry,
              "deleteFileToken: uid={} del={}", uid, del);
    return del >= 0;
}
