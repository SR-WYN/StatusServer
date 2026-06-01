# StatusServer 日志使用说明

## 1. 日志写到哪里

`config.json` 中 `Log.Dir` 为日志目录的**绝对路径**。所有 `.log` 直接放在该目录下，例如：

`/home/asus/NETLEARN/StatusServer/src/logdir/app.log`

| 枚举 `LogModule` | 文件名 | 用途 |
|------------------|--------|------|
| `App` | `app.log` | 进程启动、退出、异常（`app/StatusServer.cpp`） |
| `Config` | `config.log` | 配置加载（`infra/config/`） |
| `Grpc` | `grpc.log` | gRPC 服务 `StatusServiceImpl`、调用 Chat 节点的客户端（`infra/grpc/`） |
| `Redis` | `redis.log` | 节点表、用户绑定、登录计数等（`infra/redis/`） |
| `Registry` | `registry.log` | 聊天节点注册/注销/心跳、用户绑定节点（`application/NodeRegistry`） |

只有**实际打过日志**的模块才会生成对应文件。

---

## 2. 配置 `config.json`

```json
"Log": {
  "Dir": "/home/asus/NETLEARN/StatusServer/src/logdir",
  "Level": "info"
}
```

`Level`：`trace` / `debug` / `info` / `warn` / `error` / `critical` / `off`。

---

## 3. 初始化（`runServer` 入口，`ConfigMgr` 之后）

```cpp
#include "ConfigMgr.h"
#include "Log.h"

ConfigMgr::getInstance();
if (!Log::init("StatusServer", ConfigMgr::getInstance().getLogConfig())) {
    return;
}

// gRPC 退出后
Log::shutdown();
```

`main` 的 `catch` 里若已 `init` 过，也应调用 `Log::shutdown()`。

---

## 4. 在业务代码里打日志

```cpp
#include "Log.h"

Log::info(LogModule::Registry, "registered node {} instance {}", name, id);
Log::error(LogModule::Redis, "hSet failed key={}", key);

LOGW(LogModule::Grpc, "BuildAndStart failed");
```

### 4.1 级别

`Log::trace/debug/info/warn/error/critical`，宏 `LOGT` `LOGD` `LOGI` `LOGW` `LOGE` `LOGC`。

### 4.2 格式

使用 `{}` 占位，不要用 `%s`。

### 4.3 新增模块

新增模块：改 [`LogModule.h`](LogModule.h) 内 `LogModule` 枚举、`LogNames::_xxx` 与 `_table`（顺序一致）。

命名约定：成员变量 `_snake_case`；成员函数小写驼峰。

## 文件说明

`LogModule.h`、`Log.h`、`Log.cpp` 三个源文件；配置见 `ConfigMgr::getLogConfig()`。

---

## 5. 按目录选哪个模块（速查）

| 代码位置 | 使用 |
|----------|------|
| `src/app/` | `LogModule::App` |
| `src/infra/config/` | `LogModule::Config` |
| `src/infra/grpc/`、`StatusServiceImpl` | `LogModule::Grpc` |
| `src/infra/redis/` | `LogModule::Redis` |
| `src/application/NodeRegistry.*` | `LogModule::Registry` |

---

## 6. 注意

- `Log::init` 成功后再写日志。
- 工作目录下需有 `config.json`。
- 仅写文件；单文件持续追加，需自行清理。
