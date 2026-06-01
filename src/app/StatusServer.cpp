#include "ConfigMgr.h"
#include "Log.h"
#include "StatusServiceImpl.h"
#include <csignal>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>

// 全局 g_server 指针
static std::unique_ptr<grpc::Server> g_server;

// 关键：使用原子标志位在信号处理器和主线程间安全通信
static std::atomic<bool> g_quit{false};

void signalHandler(int signal) {
    g_quit.store(true);
}

void runServer() {
    // 1. 初始化配置与日志
    ConfigMgr::getInstance();
    if (!Log::init("StatusServer", ConfigMgr::getInstance().getLogConfig())) {
        return;
    }
    Log::info(LogModule::App, "StatusServer starting");

    // 2. 注册信号处理 (SIGINT 为 Ctrl+C)
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // 3. 构建 gRPC 服务
    auto& cfg = ConfigMgr::getInstance();
    std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
    StatusServiceImpl service;
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    g_server = builder.BuildAndStart();
    Log::info(LogModule::App, "StatusServer listening on {}", server_address);

    // 4. 核心：优雅退出的调度循环
    // 不直接调用 g_server->Wait()，而是采用轮询检测
    // 这保证了 Shutdown() 的调用方永远是主线程
    while (!g_quit.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 5. 优雅清理资源
    Log::info(LogModule::App, "Signal received, initiating graceful shutdown...");
    if (g_server) {
        g_server->Shutdown(); // 通知 gRPC 停止接受请求并完成现有处理
    }
    
    // 调用 Wait() 等待 gRPC 内部完成资源清理并返回
    // 此时 Shutdown 已经被调用，Wait 会迅速返回
    g_server->Wait();

    Log::info(LogModule::App, "StatusServer stopped safely.");
    Log::shutdown();
}

int main(int argc, char** argv) {
    try {
        runServer();
    } catch (std::exception const& e) {
        Log::error(LogModule::App, "StatusServer exception: {}", e.what());
        Log::shutdown();
        return EXIT_FAILURE;
    }
    return 0;
}