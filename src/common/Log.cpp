#include "Log.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>

#include <filesystem>
#include <utility>

namespace
{
std::filesystem::path logRoot(const LogConfig &config)
{
    std::error_code ec;
    std::filesystem::path dir = config._dir.empty() ? "src/logdir" : config._dir;
    if (dir.is_absolute())
    {
        return dir;
    }
    return std::filesystem::current_path(ec) / dir;
}
} // namespace

std::mutex Log::_mutex;
std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Log::_loggers;
LogConfig Log::_config;
std::string Log::_app_name;
bool Log::_inited = false;

bool Log::init(std::string_view app_name, const LogConfig &cfg)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_inited)
    {
        return true;
    }

    if (app_name.empty())
    {
        return false;
    }

    _app_name = std::string(app_name);
    _config = cfg;

    std::error_code ec;
    const std::filesystem::path log_root = logRoot(_config);
    std::filesystem::create_directories(log_root, ec);
    if (ec)
    {
        return false;
    }

    spdlog::set_level(_config._level);
    _inited = true;
    return true;
}

void Log::shutdown()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_inited)
    {
        return;
    }

    for (auto &entry : _loggers)
    {
        if (entry.second)
        {
            entry.second->flush();
        }
    }
    _loggers.clear();
    spdlog::shutdown();
    _inited = false;
    _app_name.clear();
}

std::shared_ptr<spdlog::logger> Log::createModuleLogger(const std::string &module_file)
{
    const std::filesystem::path log_root = logRoot(_config);
    const std::filesystem::path log_path = log_root / (module_file + ".log");
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string(), true);
    auto module_logger =
        std::make_shared<spdlog::logger>(_app_name + "." + module_file, std::move(sink));
    module_logger->set_level(_config._level);
    module_logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [%t] %v");
    module_logger->flush_on(spdlog::level::info);
    spdlog::register_logger(module_logger);
    return module_logger;
}

spdlog::logger &Log::logger(std::string_view module_file)
{
    const std::string key(module_file);

    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_inited)
        {
            static auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
            static auto fallback =
                std::make_shared<spdlog::logger>("log_not_initialized", null_sink);
            return *fallback;
        }

        auto it = _loggers.find(key);
        if (it != _loggers.end())
        {
            return *it->second;
        }
    }

    std::shared_ptr<spdlog::logger> module_logger;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _loggers.find(key);
        if (it != _loggers.end())
        {
            return *it->second;
        }
        module_logger = createModuleLogger(key);
        _loggers.emplace(key, module_logger);
    }
    return *module_logger;
}

spdlog::logger &Log::logger(LogModule module)
{
    return logger(moduleName(module));
}
