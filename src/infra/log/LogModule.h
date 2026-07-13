// LogModule.h - 日志模块枚举和名称映射
#pragma once

#include <string_view>

enum class LogModule
{
    App,
    Config,
    Grpc,
    Redis,
    Registry,
};

inline std::string_view moduleName(LogModule module)
{
    switch (module)
    {
    case LogModule::App:
        return "app";
    case LogModule::Config:
        return "config";
    case LogModule::Grpc:
        return "grpc";
    case LogModule::Redis:
        return "redis";
    case LogModule::Registry:
        return "registry";
    }
    return "unknown";
}
