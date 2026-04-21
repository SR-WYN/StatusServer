#include "MySqlDao.h"
#include "ConfigMgr.h"
#include "MySqlPool.h"
#include "utils.h"
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>

MySqlDao::MySqlDao()
{
    auto& cfg = ConfigMgr::GetInstance();
    const auto& host = cfg["MySql"]["Host"];
    const auto& port = cfg["MySql"]["Port"];
    const auto& user = cfg["MySql"]["User"];
    const auto& pass = cfg["MySql"]["Passwd"];
    const auto& schema = cfg["MySql"]["Schema"];
    _pool.reset(new MySqlPool(host + ":" + port, user, pass, schema, 5));
}

MySqlDao::~MySqlDao()
{
    _pool->Close();
}

int MySqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    auto con = _pool->GetConnection();
    try
    {
        if (con == nullptr)
        {
            return -1;
        }
        // 准备调用存储过程
        std::unique_ptr<sql::PreparedStatement> stmt(
            con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
        // 设置输入参数
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);
        // 执行存储过程
        stmt->execute();
        std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next())
        {
            int result = res->getInt("result");
            std::cout << "Result: " << result << std::endl;
            _pool->ReturnConnection(std::move(con));
            return result;
        }
        _pool->ReturnConnection(std::move(con));
        return -1;
    }
    catch (sql::SQLException& e)
    {
        _pool->ReturnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what() << std::endl;
        std::cerr << "MYSQL error code: " << e.getErrorCode() << std::endl;
        std::cerr << "SQLState: " << e.getSQLState() << std::endl;
        return -1;
    }
}

bool MySqlDao::CheckEmail(const std::string& email, const std::string& name)
{
    auto con = _pool->GetConnection();
    try
    {
        if (con == nullptr)
        {
            return false;
        }

        // 准备查询语句
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("SELECT name FROM user WHERE email = ?"));
        // 绑定参数
        pstmt->setString(1, email);
        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next())
        {
            std::cout << "Check name: " << res->getString("name") << std::endl;
            if (name == res->getString("name"))
            {
                _pool->ReturnConnection(std::move(con));
                return true;
            }
        }
        _pool->ReturnConnection(std::move(con));
        return false;
    }
    catch (sql::SQLException& e)
    {
        _pool->ReturnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what() << std::endl;
        std::cerr << "MYSQL error code: " << e.getErrorCode() << std::endl;
        std::cerr << "SQLState: " << e.getSQLState() << std::endl;
        return false;
    }
}

bool MySqlDao::UpdatePwd(const std::string& email, const std::string& pwd)
{
    auto con = _pool->GetConnection();
    try
    {
        if (con == nullptr)
        {
            return false;
        }
        // 准备查询语句
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE email = ?"));
        // 绑定参数
        pstmt->setString(1, pwd);
        pstmt->setString(2, email);
        // 执行更新
        int updateCount = pstmt->executeUpdate();

        std::cout << "Updated rows: " << updateCount << std::endl;
        _pool->ReturnConnection(std::move(con));
        return updateCount > 0;
    }
    catch (sql::SQLException& e)
    {
        _pool->ReturnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what() << std::endl;
        std::cerr << "MYSQL error code: " << e.getErrorCode() << std::endl;
        std::cerr << "SQLState: " << e.getSQLState() << std::endl;
        return false;
    }
}

bool MySqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    auto con = _pool->GetConnection();
    if (con == nullptr)
    {
        return false;
    }
    utils::Defer([this,&con](){
        _pool->ReturnConnection(std::move(con));
    });
    try 
    {
        //准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
        pstmt->setString(1,email);

        //执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";
        if (res->next())
        {
            origin_pwd = res->getString("pwd");
            // 输出查询到的密码
            std::cout << "Password: " << origin_pwd << std::endl;
        }
        if (pwd != origin_pwd)
        {
            return false;
        }
        userInfo.name = res->getString("name");
        userInfo.email = email;
        userInfo.pwd = origin_pwd;
        userInfo.uid = res->getInt("uid");
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what() << std::endl;
        std::cerr << "MYSQL error code: " << e.getErrorCode() << std::endl;
        std::cerr << "SQLState: " << e.getSQLState() << std::endl;
        return false;
    }
}