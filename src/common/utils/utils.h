// utils.h - 通用工具集合
#pragma once
#include <cstdint>
#include <string>

namespace utils::url
{

std::string encode(const std::string &str);
std::string decode(const std::string &str);

} // namespace utils::url

namespace utils::time
{

int64_t nowSec();

} // namespace utils::time

namespace utils::uuid
{

std::string generate();

} // namespace utils::uuid