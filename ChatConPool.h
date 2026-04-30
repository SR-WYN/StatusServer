#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include "message.grpc.pb.h"
#include "message.pb.h"

using grpc::Channel;
using message::ChatService;

class ChatConPool
{
public:
    ChatConPool(std::size_t pool_size, std::string host, std::string port);
    ~ChatConPool();
    void close();
    std::unique_ptr<ChatService::Stub> getConnection();
    void returnConnection(std::unique_ptr<ChatService::Stub> context);

private:
    std::atomic<bool> _b_stop;
    std::size_t _pool_size;
    std::string _host;
    std::string _port;
    std::queue<std::unique_ptr<ChatService::Stub>> _connections;
    std::mutex _mutex;
    std::condition_variable _cond;
};
