#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads = 0)
{
    workers.resize(numThreads);

    for(int i = 0; i<numThreads; i++)
    {
        workers.emplace_back([this](){
            while(1){
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock,[this]
            { 
                return !tasks.empty() || stop;
            });

            if (stop && tasks.empty())
            {
                return ;
            }
            std::function<void()> fun = std::move(tasks.front());
            tasks.pop();
            cv.notify_all();
            lock.unlock();
            fun();
        }
        });  
    }
}
void ThreadPool::pushTask(std::function<void()> func)
{
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace(std::move(func));
    }
    cv.notify_one();
}

ThreadPool::~ThreadPool()
{
    stop = true;
    for (std::thread &worker : workers)
    {
       worker.join();
    }
}
