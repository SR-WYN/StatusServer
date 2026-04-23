#pragma once
#include "data.h"
#include "MySqlPool.h"
#include <string>
#include <memory>

class MySqlPool;

class MySqlDao
{
public:
    MySqlDao();
    ~MySqlDao();
    int regUser(const std::string& name,const std::string& email,const std::string& pwd);
    bool checkEmail(const std::string& email,const std::string& name);
    bool updatePwd(const std::string& email,const std::string& pwd);
    bool checkPwd(const std::string& email,const std::string& pwd,UserInfo& userInfo);
private:
    std::unique_ptr<MySqlPool> _pool;
};