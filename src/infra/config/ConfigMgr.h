#pragma once
#include "Singleton.h"
#include <boost/filesystem.hpp>
#include <json/json.h>
#include <map>
#include <string>

// 辅助结构体：管理配置节
struct SectionInfo
{
    SectionInfo();
    ~SectionInfo();
    SectionInfo(const SectionInfo& src);
    SectionInfo& operator=(const SectionInfo& src);

    // 重载 [] 运算符方便访问 Key
    std::string operator[](const std::string& key);

    std::map<std::string, std::string> _section_datas;
};

class ConfigMgr : public Singleton<ConfigMgr>
{
    // 授予基类访问私有构造函数的权限
    friend class Singleton<ConfigMgr>;

public:
    ~ConfigMgr();

    // 重载 [] 运算符方便访问 Section
    SectionInfo operator[](const std::string& section);

private:
    // 私有构造函数，防止外部实例化
    ConfigMgr();

    // 禁止拷贝和赋值
    ConfigMgr(const ConfigMgr& src) = delete;
    ConfigMgr& operator=(const ConfigMgr& src) = delete;

    // 存储所有配置数据的 Map
    std::map<std::string, SectionInfo> _config_map;
};