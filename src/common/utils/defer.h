// defer.h - RAII 守卫，离开作用域时执行指定函数
#pragma once
#include <functional>

namespace utils
{

class Defer
{
public:
    explicit Defer(std::function<void()> func) : _func(func) {}
    ~Defer()
    {
        if (_func)
            _func();
    }
    Defer(const Defer &) = delete;
    Defer &operator=(const Defer &) = delete;

private:
    std::function<void()> _func;
};

} // namespace utils