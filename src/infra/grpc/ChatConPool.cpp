#include "ChatConPool.h"
#include <grpcpp/grpcpp.h>

ChatConPool::ChatConPool(std::size_t pool_size, std::string host, std::string port)
    : _b_stop(false), _pool_size(pool_size), _host(std::move(host)), _port(std::move(port))
{
    for (std::size_t i = 0; i < _pool_size; i++)
    {
        std::shared_ptr<Channel> channel =
            grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials());
        _connections.push(ChatService::NewStub(channel));
    }
}

ChatConPool::~ChatConPool()
{
    std::lock_guard<std::mutex> lock(_mutex);
    close();
    while (!_connections.empty())
    {
        _connections.pop();
    }
}

void ChatConPool::close()
{
    _b_stop = true;
    _cond.notify_all();
}

std::unique_ptr<ChatService::Stub> ChatConPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [this]() {
        return _b_stop || !_connections.empty();
    });
    if (_b_stop || _connections.empty())
    {
        return nullptr;
    }
    auto context = std::move(_connections.front());
    _connections.pop();
    return context;
}

void ChatConPool::returnConnection(std::unique_ptr<ChatService::Stub> context)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_b_stop)
    {
        return;
    }
    _connections.push(std::move(context));
    _cond.notify_one();
}
