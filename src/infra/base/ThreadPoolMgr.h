#pragma once
#include "Singleton.h"
#include "ThreadPool.h"
#include <functional>
#include <memory>

class ThreadPoolMgr : public Singleton<ThreadPoolMgr>
{
    friend class Singleton<ThreadPoolMgr>;

public:
    ~ThreadPoolMgr();

    // ======================== 任务队列线程池 ========================
    void enqueueRedis(std::function<void()> task);
    void enqueueGrpcClient(std::function<void()> task);

private:
    ThreadPoolMgr();

    // ======================== 任务队列线程池 ========================
    // Redis 池：处理节点注册、用户绑定、token 等 Redis 操作
    std::unique_ptr<ThreadPool> _redisPool;
    // gRPC 客户端池：处理向 GateServer 发送通知等出站 gRPC 调用
    std::unique_ptr<ThreadPool> _grpcClientPool;
};