// LogModule.h - 日志模块枚举和名称映射
#pragma once

#include <array>
#include <cstddef>
#include <string_view>

enum class LogModule
{
    App,
    Config,
    Grpc,
    Redis,
    Registry,
};

namespace LogNames
{
inline constexpr std::string_view _app = "app";
inline constexpr std::string_view _config = "config";
inline constexpr std::string_view _grpc = "grpc";
inline constexpr std::string_view _redis = "redis";
inline constexpr std::string_view _registry = "registry";

inline constexpr std::array<std::string_view, 5> _table = {
    _app, _config, _grpc, _redis, _registry,
};
} // namespace LogNames

inline std::string_view moduleName(LogModule module)
{
    return LogNames::_table[static_cast<std::size_t>(module)];
}
