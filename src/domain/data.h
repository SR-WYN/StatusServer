// data.h - 数据实体定义（用户信息结构体）
#pragma once
#include <string>

struct UserInfo
{
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
};
