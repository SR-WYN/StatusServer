// ConfigMgr.cpp - 配置管理实现，解析 JSON 配置文件并提供分节访问
#include "ConfigMgr.h"
#include "Log.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

SectionInfo::SectionInfo()
{
}

SectionInfo::~SectionInfo()
{
    _section_datas.clear();
}

SectionInfo::SectionInfo(const SectionInfo &src)
{
    _section_datas = src._section_datas;
}

SectionInfo &SectionInfo::operator=(const SectionInfo &src)
{
    if (&src == this)
    {
        return *this;
    }
    _section_datas = src._section_datas;
    return *this;
}

std::string SectionInfo::operator[](const std::string &key) const
{
    auto it = _section_datas.find(key);
    if (it == _section_datas.end())
    {
        return "";
    }
    return it->second;
}

ConfigMgr::ConfigMgr()
{
    const boost::filesystem::path config_path = boost::filesystem::current_path() / "config.json";

    std::ifstream file(config_path.string());
    if (!file.is_open())
    {
        std::cerr << "ConfigMgr: failed to open config file: " << config_path << std::endl;
        return;
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(file, root))
    {
        std::cerr << "ConfigMgr: failed to parse config JSON: "
                  << reader.getFormattedErrorMessages() << std::endl;
        return;
    }

    for (auto const &section_name : root.getMemberNames())
    {
        Json::Value section_value = root[section_name];

        if (section_value.isObject())
        {
            SectionInfo section_info;
            // 遍历该 Section 下的所有键值对
            for (auto const &key : section_value.getMemberNames())
            {
                // 统一转为 string 存储
                section_info._section_datas[key] = section_value[key].asString();
            }
            _config_map[section_name] = section_info;
        }
    }

    loadLogConfig();
    Log::info(LogModule::Config, "loaded config from {} ({} sections)", config_path.string(),
              _config_map.size());
}

namespace
{
spdlog::level::level_enum parseLogLevel(const std::string &level_str)
{
    std::string level = level_str;
    std::transform(level.begin(), level.end(), level.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (level == "trace")
    {
        return spdlog::level::trace;
    }
    if (level == "debug")
    {
        return spdlog::level::debug;
    }
    if (level == "info")
    {
        return spdlog::level::info;
    }
    if (level == "warn" || level == "warning")
    {
        return spdlog::level::warn;
    }
    if (level == "error" || level == "err")
    {
        return spdlog::level::err;
    }
    if (level == "critical" || level == "fatal")
    {
        return spdlog::level::critical;
    }
    if (level == "off")
    {
        return spdlog::level::off;
    }
    return spdlog::level::info;
}
} // namespace

void ConfigMgr::loadLogConfig()
{
    auto section = (*this)["Log"];
    const std::string dir = section["Dir"];
    if (!dir.empty())
    {
        _log_config._dir = dir;
    }
    const std::string level = section["Level"];
    if (!level.empty())
    {
        _log_config._level = parseLogLevel(level);
    }
}

LogConfig ConfigMgr::getLogConfig() const
{
    return _log_config;
}

ConfigMgr::~ConfigMgr()
{
    _config_map.clear();
}

SectionInfo ConfigMgr::operator[](const std::string &section) const
{
    auto it = _config_map.find(section);
    if (it == _config_map.end())
    {
        return SectionInfo();
    }
    return it->second;
}