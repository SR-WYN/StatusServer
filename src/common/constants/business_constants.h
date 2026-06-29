// business_constants.h - 业务配置常量
#pragma once

namespace constants::business
{

inline constexpr int kNodeTtlSeconds = 30;
inline constexpr int kTokenTtlSeconds = 300; // 登录 token TTL，默认 5 分钟，可配置

} // namespace constants::business