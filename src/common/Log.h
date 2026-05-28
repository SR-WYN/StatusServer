#pragma once

#include "LogModule.h"

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

struct LogConfig
{
    std::string _dir{"src/logdir"};
    spdlog::level::level_enum _level{spdlog::level::info};
};

class Log
{
public:
    Log() = delete;

    static bool init(std::string_view app_name, const LogConfig &cfg);
    static void shutdown();

    template <typename... Args>
    static void trace(LogModule module, fmt::format_string<Args...> fmt, Args &&...args)
    {
        logger(module).trace(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug(LogModule module, fmt::format_string<Args...> fmt, Args &&...args)
    {
        logger(module).debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(LogModule module, fmt::format_string<Args...> fmt, Args &&...args)
    {
        logger(module).info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(LogModule module, fmt::format_string<Args...> fmt, Args &&...args)
    {
        logger(module).warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(LogModule module, fmt::format_string<Args...> fmt, Args &&...args)
    {
        logger(module).error(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void critical(LogModule module, fmt::format_string<Args...> fmt, Args &&...args)
    {
        logger(module).critical(fmt, std::forward<Args>(args)...);
    }

private:
    static spdlog::logger &logger(LogModule module);
    static spdlog::logger &logger(std::string_view module_file);
    static std::shared_ptr<spdlog::logger> createModuleLogger(const std::string &module_file);

    static std::mutex _mutex;
    static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> _loggers;
    static LogConfig _config;
    static std::string _app_name;
    static bool _inited;
};

#define LOGT(mod, ...) Log::trace(mod, __VA_ARGS__)
#define LOGD(mod, ...) Log::debug(mod, __VA_ARGS__)
#define LOGI(mod, ...) Log::info(mod, __VA_ARGS__)
#define LOGW(mod, ...) Log::warn(mod, __VA_ARGS__)
#define LOGE(mod, ...) Log::error(mod, __VA_ARGS__)
#define LOGC(mod, ...) Log::critical(mod, __VA_ARGS__)
