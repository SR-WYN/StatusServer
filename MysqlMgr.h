#pragma once
#include "Singleton.h"
#include "MySqlDao.h"

class MysqlMgr : public Singleton<MysqlMgr>
{
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr() override;
    int regUser(const std::string& name,const std::string& email,const std::string& pwd);
    bool checkEmail(const std::string& email,const std::string& name);
    bool updatePwd(const std::string& email,const std::string& pwd);
    bool checkPwd(const std::string& email,const std::string& pwd,UserInfo& userInfo);
private:
    MysqlMgr();
    MySqlDao _dao;
};