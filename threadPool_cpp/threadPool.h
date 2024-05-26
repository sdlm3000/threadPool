#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <condition_variable>

#include "taskQueue.h"


class threadPool
{
public:
    threadPool(int minNum = 1, int maxNum = 5);
    ~threadPool();

    // 添加任务
    void addTask(Task task);

    // 获取忙线程个数
    int getBusyNumber();

    // 获取存活线程个数
    int getAliveNumber();

    // 获取任务队列的数量
    int getTaskNumber();
private:
    // 工作线程
    static void worker(void *arg);

    // 管理线程
    static void manager(void *arg);

    // 退出当前线程
    void exitThread();

private:
    taskQueue *m_taskQueue;
    
    std::thread managerThread;
    std::vector<std::thread> workThreads;
    int maxNum;
    int minNum;
    std::atomic<int> aliveNum;
    std::atomic<int> busyNum;
    std::atomic<int> exitNum;
    std::atomic<bool> shutdown;
    std::mutex poolLock;
    std::condition_variable poolCv;

};

