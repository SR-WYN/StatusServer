// GateNotifyClientImpl.cpp - GateServer 通知客户端实现
#include "GateNotifyClientImpl.h"
#include "Log.h"
#include "LogModule.h"
#include "const.h"

#include <chrono>
#include <grpcpp/grpcpp.h>

GateNotifyClientImpl::GateNotifyClientImpl(const std::string &address)
{
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    _stub = message::GateNotifyService::NewStub(channel);
    Log::info(LogModule::Grpc, "GateNotifyClientImpl connected to {}", address);
}

bool GateNotifyClientImpl::notifyUserOffline(int uid)
{
    const auto start = std::chrono::steady_clock::now();
    message::UserOfflineReq request;
    request.set_uid(uid);

    message::UserOfflineRsp response;
    grpc::ClientContext context;

    grpc::Status status = _stub->NotifyUserOffline(&context, request, &response);
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    if (!status.ok())
    {
        Log::error(LogModule::Grpc, "NotifyUserOffline RPC failed: uid={}, error={} cost={}ms",
                   uid, status.error_message(), cost_ms);
        return false;
    }

    if (response.error() != ErrorCodes::SUCCESS)
    {
        Log::warn(LogModule::Grpc, "NotifyUserOffline returned error: uid={}, error_code={} cost={}ms",
                  uid, response.error(), cost_ms);
        return false;
    }

    Log::info(LogModule::Grpc, "NotifyUserOffline success: uid={} cost={}ms", uid, cost_ms);
    return true;
}

bool GateNotifyClientImpl::notifyUserOnline(int uid)
{
    const auto start = std::chrono::steady_clock::now();
    message::UserOnlineReq request;
    request.set_uid(uid);

    message::UserOnlineRsp response;
    grpc::ClientContext context;

    grpc::Status status = _stub->NotifyUserOnline(&context, request, &response);
    const auto cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    if (!status.ok())
    {
        Log::error(LogModule::Grpc, "NotifyUserOnline RPC failed: uid={}, error={} cost={}ms",
                   uid, status.error_message(), cost_ms);
        return false;
    }

    if (response.error() != ErrorCodes::SUCCESS)
    {
        Log::warn(LogModule::Grpc, "NotifyUserOnline returned error: uid={}, error_code={} cost={}ms",
                  uid, response.error(), cost_ms);
        return false;
    }

    Log::info(LogModule::Grpc, "NotifyUserOnline success: uid={} cost={}ms", uid, cost_ms);
    return true;
}
