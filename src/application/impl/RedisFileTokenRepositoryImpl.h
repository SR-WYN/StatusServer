// RedisFileTokenRepositoryImpl.h — Redis 实现的文件传输临时 token 存储
#pragma once

#include "FileTokenRepository.h"

/// Redis 实现的文件传输临时 token 存储。
/// token 以 KV 形式存储在 Redis 中，key 格式为 ftoken_<uid>，支持 TTL 自动过期。
class RedisFileTokenRepositoryImpl final : public FileTokenRepository
{
public:
    RedisFileTokenRepositoryImpl() = default;
    ~RedisFileTokenRepositoryImpl() override = default;

    // 禁用拷贝和赋值
    RedisFileTokenRepositoryImpl(const RedisFileTokenRepositoryImpl&) = delete;
    RedisFileTokenRepositoryImpl& operator=(const RedisFileTokenRepositoryImpl&) = delete;

    bool saveFileToken(int uid, const std::string& token, int ttl_sec = 60) override;
    bool deleteFileToken(int uid) override;
};
