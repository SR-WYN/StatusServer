#pragma once

#include "Singleton.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <functional>
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <map>
#include <memory>
#include <unordered_map>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

enum ErrorCodes
{
    SUCCESS = 0,// 成功
    ERROR_JSON = 1001,// JSON 解析错误
    RPCFAILED = 1002,// RPC 请求错误
    VERIFY_EXPIRED = 1003,// 验证码过期
    VERIFY_CODE_ERROR = 1004,// 验证码错误
    USER_EXIST = 1005,// 用户已存在
    PASSWD_ERROR = 1006,// 密码错误
    EMAIL_NOT_MATCH = 1007,// 邮箱不匹配
    PASSWD_UP_FAILED = 1008,// 密码更新失败
    PASSWD_INVALID = 1009,// 密码无效
    PASSWD_NOT_MATCH = 1010,// 密码不匹配
};

namespace RedisPrefix {
    constexpr const char* CODE = "code_";
}
