#include "ThreadPool.h"

ThreadPool::ThreadPool(std::size_t core_num) : _stop(false)
{
    for (size_t i = 0; i < core_num; ++i)
    {
        _workers.emplace_back([this] {
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->_mutex);
                    this->_cond.wait(lock, [this] {
                        return this->_stop || !this->_tasks.empty();
                    });
                    if (this->_stop && this->_tasks.empty())
                        return;
                    task = std::move(this->_tasks.front());
                    this->_tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _stop = true;
    }
    _cond.notify_all();
    for (std::thread &worker : _workers)
        worker.join();
}

void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _tasks.emplace(std::move(task));
    }
    _cond.notify_one();
}