#pragma once

#include "RedisConPool.h"
#include "utils.h"
#include <cstring>
#include <hiredis/hiredis.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class RedisSession
{
public:
    template <typename Fn>
    static bool withConn(RedisConPool &pool, Fn &&fn)
    {
        redisContext *ctx = pool.getConnection();
        if (!ctx)
        {
            return false;
        }
        utils::Defer defer([&]() { pool.returnConnection(ctx); });
        return fn(ctx);
    }

    static bool get(RedisConPool &pool, const std::string &key, std::string &val)
    {
        val.clear();
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"GET", key});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type != REDIS_REPLY_STRING)
            {
                return false;
            }
            val = reply->str;
            return true;
        });
    }

    static bool set(RedisConPool &pool, const std::string &key, const std::string &val)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"SET", key, val});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_STATUS &&
                   (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0);
        });
    }

    static bool existsKey(RedisConPool &pool, const std::string &key)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"EXISTS", key});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
        });
    }

    static bool del(RedisConPool &pool, const std::string &key)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"DEL", key});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER;
        });
    }

    static std::string hGet(RedisConPool &pool, const std::string &key, const std::string &field)
    {
        std::string val;
        withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"HGET", key, field});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type == REDIS_REPLY_NIL)
            {
                return true;
            }
            if (reply->type == REDIS_REPLY_ERROR)
            {
                std::cerr << "HGET error: " << reply->str << std::endl;
                return false;
            }
            if (reply->type == REDIS_REPLY_STRING)
            {
                val = reply->str;
            }
            return true;
        });
        return val;
    }

    static bool hSet(RedisConPool &pool, const std::string &key, const std::string &field,
                     const std::string &val)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"HSET", key, field, val});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER;
        });
    }

    static bool hDel(RedisConPool &pool, const std::string &key, const std::string &field)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"HDEL", key, field});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER;
        });
    }

    static bool hGetAll(RedisConPool &pool, const std::string &key,
                        std::map<std::string, std::string> &out)
    {
        out.clear();
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"HGETALL", key});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type != REDIS_REPLY_ARRAY)
            {
                return false;
            }
            for (size_t i = 0; i + 1 < reply->elements; i += 2)
            {
                if (reply->element[i]->type == REDIS_REPLY_STRING &&
                    reply->element[i + 1]->type == REDIS_REPLY_STRING)
                {
                    out[reply->element[i]->str] = reply->element[i + 1]->str;
                }
            }
            return true;
        });
    }

    static bool sAdd(RedisConPool &pool, const std::string &key, const std::string &member)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"SADD", key, member});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER;
        });
    }

    static bool sRem(RedisConPool &pool, const std::string &key, const std::string &member)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"SREM", key, member});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER;
        });
    }

    static bool sMembers(RedisConPool &pool, const std::string &key, std::vector<std::string> &out)
    {
        out.clear();
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"SMEMBERS", key});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type != REDIS_REPLY_ARRAY)
            {
                return false;
            }
            for (size_t i = 0; i < reply->elements; ++i)
            {
                if (reply->element[i]->type == REDIS_REPLY_STRING)
                {
                    out.emplace_back(reply->element[i]->str);
                }
            }
            return true;
        });
    }

    static bool auth(RedisConPool &pool, const std::string &password)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"AUTH", password});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type != REDIS_REPLY_ERROR;
        });
    }

    static bool lPush(RedisConPool &pool, const std::string &key, const std::string &val)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"LPUSH", key, val});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
        });
    }

    static bool lPop(RedisConPool &pool, const std::string &key, std::string &val)
    {
        val.clear();
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"LPOP", key});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type == REDIS_REPLY_NIL)
            {
                return false;
            }
            if (reply->type != REDIS_REPLY_STRING)
            {
                return false;
            }
            val = reply->str;
            return true;
        });
    }

    static bool rPush(RedisConPool &pool, const std::string &key, const std::string &val)
    {
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"RPUSH", key, val});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            return reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
        });
    }

    static bool rPop(RedisConPool &pool, const std::string &key, std::string &val)
    {
        val.clear();
        return withConn(pool, [&](redisContext *ctx) {
            redisReply *reply = execArgv(ctx, {"RPOP", key});
            if (!reply)
            {
                return false;
            }
            ReplyGuard guard(reply);
            if (reply->type == REDIS_REPLY_NIL)
            {
                return false;
            }
            if (reply->type != REDIS_REPLY_STRING)
            {
                return false;
            }
            val = reply->str;
            return true;
        });
    }

private:
    struct ReplyGuard
    {
        redisReply *reply = nullptr;
        explicit ReplyGuard(redisReply *r) : reply(r) {}
        ~ReplyGuard()
        {
            if (reply)
            {
                freeReplyObject(reply);
            }
        }
        ReplyGuard(const ReplyGuard &) = delete;
        ReplyGuard &operator=(const ReplyGuard &) = delete;
    };

    static redisReply *execArgv(redisContext *ctx, const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            return nullptr;
        }
        std::vector<const char *> argv;
        std::vector<size_t> arglen;
        argv.reserve(args.size());
        arglen.reserve(args.size());
        for (const auto &arg : args)
        {
            argv.push_back(arg.c_str());
            arglen.push_back(arg.size());
        }
        redisReply *reply = static_cast<redisReply *>(
            redisCommandArgv(ctx, static_cast<int>(argv.size()), argv.data(), arglen.data()));
        if (!reply)
        {
            std::cerr << "Redis command failed: " << ctx->errstr << std::endl;
        }
        return reply;
    }
};
