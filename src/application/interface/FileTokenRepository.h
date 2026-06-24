// FileTokenRepository.h — 文件传输临时 token 存储接口
// 与 NodeRegistry 职责分离，专用于管理文件上传/下载的临时授权 token。
#pragma once
#include <string>

/// 文件传输临时 token 存储接口
/// 负责生成、保存、删除文件传输场景下的临时 token（如上传图片时的 Bearer token）。
class FileTokenRepository
{
public:
    virtual ~FileTokenRepository() = default;

    /// 保存文件传输临时 token，并设置过期时间。
    /// @param uid       用户 ID
    /// @param token     临时 token 字符串
    /// @param ttl_sec   过期时间（秒）
    /// @return 是否保存成功
    virtual bool saveFileToken(int uid, const std::string& token, int ttl_sec = 60) = 0;

    /// 删除指定用户的文件传输临时 token。
    /// @param uid 用户 ID
    /// @return 是否删除成功（key 不存在也视为成功）
    virtual bool deleteFileToken(int uid) = 0;
};
