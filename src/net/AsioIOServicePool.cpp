#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;
AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _io_services(size),
      _works(size),
      _next_io_service(0)
{
    for (std::size_t i = 0; i < size; ++i)
    {
        _works[i] = std::unique_ptr<Work>(new Work(_io_services[i]));
    }
    // 遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
    for (std::size_t i = 0; i < _io_services.size(); ++i)
    {
        _threads.emplace_back(
            [this, i]()
            {
                _io_services[i].run();
            });
    }
}
AsioIOServicePool::~AsioIOServicePool()
{
    stop();
    std::cout << "AsioIOServicePool destruct" << endl;
}
boost::asio::io_context &AsioIOServicePool::getIoService()
{
    auto& service = _io_services[_next_io_service++];
    if (_next_io_service == _io_services.size())
    {
        _next_io_service = 0;
    }
    return service;
}
void AsioIOServicePool::stop()
{
    // 因为仅仅执行work.reset并不能让iocontext从run的状态中退出
    // 当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
    for (auto& work : _works)
    {
        // 把服务先停止
        work->get_io_context().stop();
        work.reset();
    }
    for (auto& t : _threads)
    {
        t.join();
    }
}