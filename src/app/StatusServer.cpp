#include "ConfigMgr.h"
#include "Log.h"
#include "StatusServiceImpl.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <string>

void runServer();

int main(int argc, char** argv)
{
    try
    {
        runServer();
    }
    catch (std::exception const& e)
    {
        Log::error(LogModule::App, "StatusServer exception: {}", e.what());
        Log::shutdown();
        return EXIT_FAILURE;
    }
    return 0;
}

void runServer()
{
    ConfigMgr::getInstance();
    if (!Log::init("StatusServer", ConfigMgr::getInstance().getLogConfig()))
    {
        return;
    }
    Log::info(LogModule::App, "StatusServer starting");

    auto& cfg = ConfigMgr::getInstance();
    std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
    StatusServiceImpl service;
    grpc::ServerBuilder builder;
    // 监听端口和添加服务
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    // 构建并启动gRPC服务器
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    Log::info(LogModule::App, "StatusServer listening on {}", server_address);
    // 等待服务器关闭
    server->Wait();
    Log::info(LogModule::App, "StatusServer stopped");
    Log::shutdown();
}