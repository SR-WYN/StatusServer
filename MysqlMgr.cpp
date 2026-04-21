#include "MysqlMgr.h"

MysqlMgr::MysqlMgr()
{
}

MysqlMgr::~MysqlMgr()
{
}

int MysqlMgr::RegUser(const std::string& name,const std::string& email,const std::string& pwd)
{
    return _dao.RegUser(name,email,pwd);
}

bool MysqlMgr::CheckEmail(const std::string& email,const std::string& name)
{
    return _dao.CheckEmail(email,name);
}

bool MysqlMgr::UpdatePwd(const std::string& email,const std::string& pwd)
{
    return _dao.UpdatePwd(email,pwd);
}

bool MysqlMgr::CheckPwd(const std::string& email,const std::string& pwd,UserInfo& userInfo)
{
    return _dao.CheckPwd(email,pwd,userInfo);
}
