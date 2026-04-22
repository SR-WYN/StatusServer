#pragma once
#include <atomic>
#include <condition_variable>
#include <hiredis/hiredis.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

class RedisConPool
{
public:
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd);
    ~RedisConPool();
    void clearConnections();
    redisContext* getConnection();
    redisContext* getConNonBlock();
    void returnConnection(redisContext* context);
    void close();

private:
    bool reconnect();
    void checkThreadPro();
    std::atomic<bool> _b_stop;
    size_t _pool_size;
    std::string _host;
    std::string _pwd;
    int _port;
    std::queue<redisContext*> _connections;
    std::atomic<int> _fail_count;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::thread _check_thread;
    int _counter;
};