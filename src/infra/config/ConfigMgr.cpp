#include "ConfigMgr.h"
#include <fstream>
#include <iostream>

SectionInfo::SectionInfo()
{
}

SectionInfo::~SectionInfo()
{
    _section_datas.clear();
}

SectionInfo::SectionInfo(const SectionInfo& src)
{
    _section_datas = src._section_datas;
}

SectionInfo& SectionInfo::operator=(const SectionInfo& src)
{
    if (&src == this)
    {
        return *this;
    }
    _section_datas = src._section_datas;
    return *this;
}

std::string SectionInfo::operator[](const std::string& key)
{
    if (_section_datas.find(key) == _section_datas.end())
    {
        return "";
    }
    return _section_datas.at(key);
}

ConfigMgr::ConfigMgr()
{
    // 获取当前工作目录并构建配置文件路径
    boost::filesystem::path current_path = boost::filesystem::current_path();
    boost::filesystem::path config_path = current_path / "config.json";

    std::cout << "Loading config from: " << config_path << std::endl;

    std::ifstream file(config_path.string());
    if (!file.is_open())
    {
        std::cerr << "Config file not found!" << std::endl;
        return;
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(file, root))
    {
        std::cerr << "JSON parse error!" << std::endl;
        return;
    }

    for (auto const& section_name : root.getMemberNames())
    {
        Json::Value section_value = root[section_name];

        if (section_value.isObject())
        {
            SectionInfo section_info;
            // 遍历该 Section 下的所有键值对
            for (auto const& key : section_value.getMemberNames())
            {
                // 统一转为 string 存储
                section_info._section_datas[key] = section_value[key].asString();
            }
            _config_map[section_name] = section_info;
        }
    }

    // 打印加载结果用于验证
    for (const auto& pair : _config_map)
    {
        std::cout << "Section [" << pair.first << "] loaded." << std::endl;
    }
}

ConfigMgr::~ConfigMgr()
{
    _config_map.clear();
}

SectionInfo ConfigMgr::operator[](const std::string& section)
{
    if (_config_map.find(section) == _config_map.end())
    {
        return SectionInfo();
    }
    return _config_map[section];
}