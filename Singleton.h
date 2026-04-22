#pragma once
#include <iostream>

template <typename T>
class Singleton
{
protected:
    // 保护构造：允许子类构造，禁止外部构造
    Singleton() = default;
    virtual ~Singleton()
    {
        std::cout << "this is singleton destruct" << std::endl;
    }

    // 禁止拷贝和赋值
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

public:
    static T &getInstance()
    {
        static T instance;
        return instance;
    }

    void printAddress()
    {
        std::cout << &getInstance() << std::endl;
    }
};
