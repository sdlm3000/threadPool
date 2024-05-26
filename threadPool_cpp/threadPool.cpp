#include <iostream>
#include <string.h>
#include <unistd.h>

#include "threadPool.h"

#define VAR_NUMBER      2

threadPool::threadPool(int minNum, int maxNum)
    :minNum(minNum)
    ,maxNum(maxNum)
    ,shutdown(false)
    ,busyNum(0)
    ,exitNum(0)
{
    m_taskQueue = new taskQueue;
    if(minNum <= 0) {
        std::cout << "minNum error!" << std::endl;
        minNum = 1;
    }
    if(maxNum < minNum) {
        std::cout << "maxNum error!" << std::endl;
        maxNum = 5;
    }
    aliveNum = minNum;
    // TODO: 初始化的异常没处理
    for(int i = 0; i < aliveNum; i++) {
        workThreads.push_back(std::thread(worker, this));
        std::cout << "worker thread " << workThreads.back().get_id() << " add in group" << std::endl;
        std::cout << "worker thread " << workThreads.back().get_id() << " add in group" << std::endl;
        workThreads.back().detach();
    }
    managerThread = std::thread(manager, this);
    std::cout << "threadPoll create..." << std::endl;

}
threadPool::~threadPool()
{
    shutdown = true;
    if(managerThread.joinable()) {
        managerThread.join();
    }
    for(int i = 0; i < workThreads.size(); i++) {
        poolCv.notify_one();
    }
    if(m_taskQueue != nullptr) {
        delete m_taskQueue;
        m_taskQueue = nullptr;
    }
    std::cout << "threadPoll destroy..." << std::endl;


}

// 添加任务
void threadPool::addTask(Task task)
{
    m_taskQueue->addTask(task);
    poolCv.notify_one();
}

// 获取忙线程个数
int threadPool::getBusyNumber()
{
    return busyNum;
}

// 获取存活线程个数
int threadPool::getAliveNumber()
{
    return aliveNum;

}

// 获取任务队列的数量
int threadPool::getTaskNumber()
{
    return m_taskQueue->getTaskNumber();
}

// 工作线程
void threadPool::worker(void *arg)
{
    threadPool *pool = static_cast<threadPool*>(arg);
    while (true)
    {
        // 上锁
        {
            std::unique_lock<std::mutex> lock(pool->poolLock);
            while(pool->m_taskQueue->getTaskNumber() == 0 && pool->shutdown == 0) {
                pool->poolCv.wait(lock);
                // 当exitNum大于0，退出当前线程
                if(pool->exitNum > 0) {
                    pool->exitNum--;
                    pool->aliveNum--;
                    pool->exitThread();
                    return;
                }
            }
        }
        // 当shutdown为true时，退出当前线程
        if(pool->shutdown) {
            pool->exitThread();
            return;
        }
        // 给该线程分配任务
        Task t = pool->m_taskQueue->takeTask();

        pool->busyNum++;
        t.function(t.arg);
        if(t.arg != nullptr) {
            delete t.arg;
            // free(t.arg);
            t.arg = nullptr;
        }
        
        pool->busyNum--;
    }

}

// 管理线程
void threadPool::manager(void *arg)
{
    threadPool *pool = static_cast<threadPool*>(arg);
    std::cout << "manager thread " << std::this_thread::get_id() << " begin..." << std::endl;
    while(!pool->shutdown) {
        sleep(3);
        int aliveNum = pool->aliveNum;
        int busyNum = pool->busyNum;
        int queueSize = pool->m_taskQueue->getTaskNumber();
        // TODO: 线程管理策略还需要进一步优化
        // 当存活线程小于队列中数量 && 存活线程数小于最大线程数
        if(aliveNum < queueSize && aliveNum < pool->maxNum) {
            for(int count = 0; pool->aliveNum < pool->maxNum &&
                    count < VAR_NUMBER; count++) {
                pool->aliveNum++;
                pool->workThreads.push_back(std::thread(worker, pool));
                std::cout << "worker thread " << pool->workThreads.back().get_id()
                            << " add in group" << std::endl;
                pool->workThreads.back().detach();
                
             }
        }
        // 当忙线程数的两倍小于存活线程数 && 存活线程数大于最小线程数
        if(busyNum * 2 < aliveNum && aliveNum > pool->minNum) {
            pool->exitNum += VAR_NUMBER;
            pool->poolCv.notify_all();
        }
    }
    std::cout << "manager thread" << std::this_thread::get_id() << " end..." << std::endl;
}

// 退出当前线程
void threadPool::exitThread()
{
    // 清除容器中结束的线程
    auto it = workThreads.begin();
    auto currentTid = std::this_thread::get_id();
    std::cout << "debug :: worker thread " << currentTid << std::endl;
    while(it != workThreads.end()) {
        auto tid = (*it).get_id();
        std::cout << "debug :: get thread " << tid << std::endl;
        if(tid == currentTid) {
            workThreads.erase(it);
            std::cout << "worker thread " << currentTid << " exit from group" << std::endl;
            break;
        }
        ++it;
    }
}