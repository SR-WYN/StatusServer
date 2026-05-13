#include "MysqlMgr.h"

MysqlMgr::MysqlMgr()
{
}

MysqlMgr::~MysqlMgr()
{
}

int MysqlMgr::regUser(const std::string& name,const std::string& email,const std::string& pwd)
{
    return _dao.regUser(name,email,pwd);
}

bool MysqlMgr::checkEmail(const std::string& email,const std::string& name)
{
    return _dao.checkEmail(email,name);
}

bool MysqlMgr::updatePwd(const std::string& email,const std::string& pwd)
{
    return _dao.updatePwd(email,pwd);
}

bool MysqlMgr::checkPwd(const std::string& email,const std::string& pwd,UserInfo& userInfo)
{
    return _dao.checkPwd(email,pwd,userInfo);
}
