add_library(ss_mysql STATIC MySqlPool.cpp MySqlDao.cpp MysqlMgr.cpp)
target_link_libraries(ss_mysql PUBLIC statusserver_includes ss_config ss_common mysqlcppconn)
