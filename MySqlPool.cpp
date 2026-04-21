#include "MySqlPool.h"
#include "utils.h"
#include <cppconn/exception.h>
#include <mutex>
#include <mysql_driver.h>

SqlConnection::SqlConnection(sql::Connection* con, int64_t lasttime)
    : _con(con),
      _last_oper_time(lasttime)
{
}

MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pass,
                     const std::string& schema, int poolSize)
    : _url(url),
      _user(user),
      _pass(pass),
      _schema(schema),
      _poolSize(poolSize),
      _b_stop(false),
      _fail_count(0)
{
    try
    {
        for (int i = 0; i < _poolSize; i++)
        {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            auto* con = driver->connect(_url, _user, _pass);
            con->setSchema(_schema);
            // 获取当前时间戳
            auto currentTime = std::chrono::system_clock::now().time_since_epoch();
            // 将时间戳转换为秒
            long long timestamp =
                std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
            _pool.push(std::make_unique<SqlConnection>(con, timestamp));
        }
        _check_thread = std::thread(
            [this]()
            {
                while (!_b_stop)
                {
                    CheckConnection();
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                }
            });
    }
    catch (sql::SQLException& e)
    {
        std::cout << "mysql pool init failed,error is " << e.what() << std::endl;
    }
}

void MySqlPool::CheckConnection()
{
    size_t targetCount;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        targetCount = _pool.size();
    }

    auto now = std::chrono::system_clock::now().time_since_epoch();
    long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    for (size_t i = 0; i < targetCount; ++i)
    {
        std::unique_ptr<SqlConnection> con;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_pool.empty())
                break;
            con = std::move(_pool.front());
            _pool.pop();
        }

        bool healthy = true;
        utils::Defer defer(
            [&]
            {
                if (healthy && con)
                {
                    std::lock_guard<std::mutex> lock(_mutex);
                    _pool.push(std::move(con));
                    _cond.notify_one();
                }
            });

        // 检查活跃度
        if (timestamp - con->_last_oper_time >= 5)
        {
            try
            {
                std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
                stmt->executeQuery("SELECT 1");
                con->_last_oper_time = timestamp;
            }
            catch (sql::SQLException& e)
            {
                std::cout << "MySQL KeepAlive Error: " << e.what() << std::endl;
                healthy = false; // 标记为坏连接，DEFER 此时不会将其还回池子
                _fail_count++;
            }
        }
    }

    // 重连补偿：确保把缺额补齐，去掉那个多余的 break
    while (_fail_count > 0)
    {
        if (reconnect(timestamp))
        {
            _fail_count--;
        }
        else
        {
            break; // 确实连不上了（如网络断开），退出等下次检查
        }
    }
}

bool MySqlPool::reconnect(long long timestamp)
{
    try
    {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        auto* con = driver->connect(_url, _user, _pass);
        con->setSchema(_schema);

        auto newCon = std::make_unique<SqlConnection>(con, timestamp);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pool.push(std::move(newCon));
        }

        std::cout << "mysql connection reconnect success" << std::endl;
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cout << "Reconnect failed,error is : " << e.what() << std::endl;
        return false;
    }
}

std::unique_ptr<SqlConnection> MySqlPool::GetConnection()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock,
               [this]()
               {
                   if (_b_stop)
                   {
                       return true;
                   }
                   return !_pool.empty();
               });
    if (_b_stop)
    {
        return nullptr;
    }
    std::unique_ptr<SqlConnection> con(std::move(_pool.front()));
    _pool.pop();
    return con;
}

void MySqlPool::ReturnConnection(std::unique_ptr<SqlConnection> con)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_b_stop)
    {
        return;
    }
    _pool.push(std::move(con));
    _cond.notify_one();
}

void MySqlPool::Close()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _b_stop = true;
    _cond.notify_all();
}

MySqlPool::~MySqlPool()
{
    std::unique_lock<std::mutex> lock(_mutex);
    while (!_pool.empty())
    {
        _pool.pop();
    }
}
