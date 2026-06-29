// utils.cpp - 通用工具集合实现
#include "utils.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>

namespace utils::url
{

static unsigned char toHex(unsigned char x)
{
    return x > 9 ? x + 55 : x + 48;
}

static unsigned char fromHex(unsigned char x)
{
    if (x >= 'A' && x <= 'F')
        return x - 'A' + 10;
    else if (x >= 'a' && x <= 'f')
        return x - 'a' + 10;
    else if (x >= '0' && x <= '9')
        return x - '0';
    else
        return 0;
}

std::string encode(const std::string &str)
{
    std::string strTemp = "";
    for (unsigned char c : str)
    {
        if (std::isalnum(c) || (c == '-') || (c == '_') || (c == '.') || (c == '~'))
        {
            strTemp += c;
        }
        else if (c == ' ')
        {
            strTemp += "%20";
        }
        else
        {
            strTemp += '%';
            strTemp += toHex(c >> 4);
            strTemp += toHex(c & 0x0F);
        }
    }
    return strTemp;
}

std::string decode(const std::string &str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+')
        {
            strTemp += ' ';
        }
        else if (str[i] == '%' && i + 2 < length)
        {
            unsigned char high = fromHex(str[++i]);
            unsigned char low = fromHex(str[++i]);
            strTemp += static_cast<unsigned char>((high << 4) | low);
        }
        else
        {
            strTemp += str[i];
        }
    }
    return strTemp;
}

} // namespace utils::url

namespace utils::time
{

int64_t nowSec()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // namespace utils::time

namespace utils::uuid
{

std::string generate()
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return to_string(uuid);
}

} // namespace utils::uuid