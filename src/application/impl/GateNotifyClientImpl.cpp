// GateNotifyClientImpl.cpp - GateServer 通知客户端实现
#include "GateNotifyClientImpl.h"
#include "Log.h"
#include "LogModule.h"
#include "const.h"

#include <grpcpp/grpcpp.h>

GateNotifyClientImpl::GateNotifyClientImpl(const std::string &address)
{
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    _stub = message::GateNotifyService::NewStub(channel);
    Log::info(LogModule::Grpc, "GateNotifyClientImpl connected to {}", address);
}

bool GateNotifyClientImpl::notifyUserOffline(int uid)
{
    message::UserOfflineReq request;
    request.set_uid(uid);

    message::UserOfflineRsp response;
    grpc::ClientContext context;

    grpc::Status status = _stub->NotifyUserOffline(&context, request, &response);
    if (!status.ok())
    {
        Log::error(LogModule::Grpc, "NotifyUserOffline failed: uid={}, error={}", uid,
                   status.error_message());
        return false;
    }

    if (response.error() != ErrorCodes::SUCCESS)
    {
        Log::warn(LogModule::Grpc, "NotifyUserOffline returned error: uid={}, error_code={}", uid,
                  response.error());
        return false;
    }

    Log::info(LogModule::Grpc, "NotifyUserOffline success: uid={}", uid);
    return true;
}
