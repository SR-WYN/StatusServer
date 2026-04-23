#pragma once
#include <atomic>
#include <condition_variable>
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class SqlConnection
{
public:
    SqlConnection(sql::Connection* con, int64_t lasttime);
    std::unique_ptr<sql::Connection> _con;
    int64_t _last_oper_time;
};

class MySqlPool
{
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass,
              const std::string& schema, int poolSize);
    std::unique_ptr<SqlConnection> getConnection();
    void returnConnection(std::unique_ptr<SqlConnection> con);
    void close();
    ~MySqlPool();
    void checkConnection();
    bool reconnect(long long timestamp);
private:
    std::string _url;
    std::string _user;
    std::string _pass;
    std::string _schema;
    int _pool_size;
    std::queue<std::unique_ptr<SqlConnection>> _pool;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::atomic<bool> _b_stop;
    std::thread _check_thread;
    std::atomic<int> _fail_count;
};