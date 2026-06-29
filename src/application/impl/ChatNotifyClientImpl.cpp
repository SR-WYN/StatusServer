// ChatNotifyClientImpl.cpp - ChatServer 通知客户端实现
#include "ChatNotifyClientImpl.h"
#include "Log.h"
#include "LogModule.h"
#include "const.h"

#include <chrono>
#include <grpcpp/client_context.h>
#include <grpcpp/grpcpp.h>

namespace
{
std::string endpointKey(const std::string &host, const std::string &port)
{
    return host + ":" + port;
}
} // anonymous namespace

std::shared_ptr<message::ChatService::Stub> ChatNotifyClientImpl::getOrCreateStub(
    const std::string &rpc_host, const std::string &rpc_port)
{
    if (rpc_host.empty() || rpc_port.empty())
    {
        return nullptr;
    }

    const std::string key = endpointKey(rpc_host, rpc_port);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _stubs.find(key);
        if (it != _stubs.end())
        {
            return it->second;
        }
    }

    auto channel = grpc::CreateChannel(key, grpc::InsecureChannelCredentials());
    auto stub = message::ChatService::NewStub(channel);
    auto shared_stub = std::shared_ptr<message::ChatService::Stub>(stub.release());

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stubs[key] = shared_stub;
    }
    Log::info(LogModule::Grpc, "ChatNotifyClientImpl: created stub {}", key);
    return shared_stub;
}

bool ChatNotifyClientImpl::notifyKickUser(const std::string &rpc_host,
                                          const std::string &rpc_port,
                                          int uid, const std::string &reason)
{
    const auto start = std::chrono::steady_clock::now();
    auto stub = getOrCreateStub(rpc_host, rpc_port);
    if (!stub)
    {
        Log::warn(LogModule::Grpc, "notifyKickUser: invalid endpoint uid={} host={} port={}",
                  uid, rpc_host, rpc_port);
        return false;
    }

    message::KickUserReq request;
    request.set_uid(uid);
    request.set_reason(reason);

    message::KickUserRsp response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(2));

    grpc::Status status = stub->KickUser(&context, request, &response);
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    if (!status.ok())
    {
        Log::warn(LogModule::Grpc,
                  "notifyKickUser RPC failed: uid={} endpoint={}:{} code={} msg={} cost={}ms",
                  uid, rpc_host, rpc_port, static_cast<int>(status.error_code()),
                  status.error_message(), cost_ms);
        return false;
    }

    if (response.error() != ErrorCodes::SUCCESS)
    {
        Log::warn(LogModule::Grpc,
                  "notifyKickUser returned error: uid={} endpoint={}:{} error={} cost={}ms",
                  uid, rpc_host, rpc_port, response.error(), cost_ms);
        return false;
    }

    Log::info(LogModule::Grpc, "notifyKickUser success: uid={} endpoint={}:{} cost={}ms",
              uid, rpc_host, rpc_port, cost_ms);
    return true;
}
