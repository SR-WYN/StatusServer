#include "RedisConPool.h"
#include <exception>
#include <hiredis/hiredis.h>
#include <hiredis/read.h>

RedisConPool::RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
    : _pool_size(poolSize),
      _host(host),
      _port(port),
      _b_stop(false),
      _pwd(pwd),
      _counter(0),
      _fail_count(0)
{
    for (size_t i = 0; i < _pool_size; ++i)
    {
        auto* context = redisConnect(host, port);
        if (context == nullptr || context->err != 0)
        {
            if (context != nullptr)
            {
                redisFree(context);
            }
            continue;
        }

        auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
        {
            std::cout << reply->str << std::endl;
            std::cout << "认证失败" << std::endl;
            // 执行成功 释放redisCommand执行后返回的redisReply所占用的内存
            freeReplyObject(reply);
            continue;
        }

        // 执行成功 释放redisCommand执行后返回的redisReply所占用的内存
        freeReplyObject(reply);
        std::cout << "认证成功" << std::endl;
        _connections.push(context);
    }

    _check_thread = std::thread(
        [this]()
        {
            while (!_b_stop)
            {
                _counter++;
                if (_counter >= 60)
                {
                    checkThreadPro();
                    _counter = 0;
                }

                std::this_thread::sleep_for(
                    std::chrono::seconds(1)); // 每隔 30 秒发送一次 PING 命令
            }
        });
}

RedisConPool::~RedisConPool()
{
    close();
    clearConnections();
}

void RedisConPool::clearConnections()
{
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_connections.empty())
    {
        auto* context = _connections.front();
        redisFree(context);
        _connections.pop();
    }
}

redisContext* RedisConPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock,
               [this]
               {
                   if (_b_stop)
                   {
                       return true;
                   }
                   return !_connections.empty();
               });
    if (_b_stop)
    {
        return nullptr;
    }
    auto* context = _connections.front();
    _connections.pop();
    return context;
}

redisContext* RedisConPool::getConNonBlock()
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop)
    {
        return nullptr;
    }
    if (_connections.empty())
    {
        return nullptr;
    }
    auto* context = _connections.front();
    _connections.pop();
    return context;
}

void RedisConPool::returnConnection(redisContext* context)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_b_stop)
    {
        return;
    }
    _connections.push(context);
    _cond.notify_one();
}

void RedisConPool::close()
{
    _b_stop = true;
    _cond.notify_all();
    if (_check_thread.joinable()) {
        _check_thread.join();
    }
}

bool RedisConPool::reconnect()
{
    auto context = redisConnect(_host.c_str(), _port);
    if (context == nullptr || context->err != 0)
    {
        if (context != nullptr)
        {
            redisFree(context);
        }
        return false;
    }
    auto reply = (redisReply*)redisCommand(context, "AUTH %s", _pwd.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
        std::cout << "认证失败" << std::endl;
        if (reply)
        {
            freeReplyObject(reply);
        }
        redisFree(context);
        return false;
    }
    // 执行成功 释放redisCommand执行后返回的redisReply所占用的内存
    freeReplyObject(reply);
    std::cout << "认证成功" << std::endl;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _connections.push(context);
    }
    return true;
}

void RedisConPool::checkThreadPro()
{
    size_t poolSize;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        poolSize = _connections.size();
    }
    for (int i = 0; i < poolSize && !_b_stop; i++)
    {
        auto* context = getConNonBlock();
        if (context == nullptr)
        {
            break;
        }
        auto reply = (redisReply*)redisCommand(context, "PING");
        if (context->err)
        {
            std::cout << "Connection error: " << context->err << std::endl;
            if (reply)
            {
                freeReplyObject(reply);
            }
            redisFree(context);
            _fail_count++;
            continue;
        }
        if (!reply || reply->type == REDIS_REPLY_ERROR)
        {
            std::cout << "reply is nullptr,redis ping failed: " << std::endl;
            if (reply)
            {
                freeReplyObject(reply);
            }
            redisFree(context);
            _fail_count++;
            continue;
        }
        freeReplyObject(reply);
        returnConnection(context);
    }
    while (_fail_count > 0)
    {
        if (reconnect())
        {
            _fail_count--;
        }
        else
        {
            break;
        }
    }
}

