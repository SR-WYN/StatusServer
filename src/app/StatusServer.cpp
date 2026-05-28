#include "AsioIOServicePool.h"
#include "ConfigMgr.h"
#include "Log.h"
#include "StatusServiceImpl.h"
#include <boost/asio.hpp>
#include <hiredis/hiredis.h>
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>

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
    // 创建Boost.Asio的io_context
    boost::asio::io_context io_context;
    // 创建signal_set用于捕获SIGINT
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    // 设置异步等待SIGINT信号
    signals.async_wait(
        [&server](const boost::system::error_code& error, int signal_number)
        {
            if (!error)
            {
                server->Shutdown(); // 优雅地关闭服务器
            }
        });
    // 在单独的线程中运行io_context
    std::thread(
        [&io_context]()
        {
            io_context.run();
        })
        .detach();
    // 等待服务器关闭
    server->Wait();
    io_context.stop(); // 停止io_context
    Log::info(LogModule::App, "StatusServer stopped");
    Log::shutdown();
}