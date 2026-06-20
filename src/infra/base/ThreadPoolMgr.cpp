#include "ThreadPoolMgr.h"

ThreadPoolMgr::ThreadPoolMgr()
{
    // Redis 池 2 线程：StatusServer 以 Redis 操作为主，低并发够用
    _redisPool = std::make_unique<ThreadPool>(2);
    // gRPC 客户端池 2 线程：处理向 GateServer 的出站通知
    _grpcClientPool = std::make_unique<ThreadPool>(2);
}

ThreadPoolMgr::~ThreadPoolMgr() = default;

void ThreadPoolMgr::enqueueRedis(std::function<void()> task)
{
    _redisPool->enqueue(std::move(task));
}

void ThreadPoolMgr::enqueueGrpcClient(std::function<void()> task)
{
    _grpcClientPool->enqueue(std::move(task));
}