#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__
#include <pthread.h>
typedef struct _task {
    void (*function)(void *arg);
    void *arg;
}task;


typedef struct _threadPool
{
    task *taskQueue;    // 任务队列
    int queueCapacity;  // 队列容量
    int queueSize;      // 队列大小
    int queueFront;     // 队头
    int queueTail;      // 队尾
    pthread_t managerID;
    pthread_t *workIDs;
    pthread_mutex_t mutexPool;      // 线程池的互斥锁
    pthread_mutex_t mutexBusy;      // busyNum的互斥锁
    pthread_cond_t taskNotFull;     // 任务数量满 
    pthread_cond_t taskNotEmpty;    // 任务数量空
    int maxNum;         // 最大线程数量
    int minNum;         // 最小线程数量
    int aliveNum;       // 当前存在的线程数量
    int busyNum;        // 当前正在工作的线程数量
    int exitNum;        // 要退出的线程数量
    int shutdown;       // 线程池退出的标志位，1代表退出
}threadPool;


// 创建线程池
threadPool *threadPoolCreate(int min, int max, int queueSzie);

// 销毁线程池
int threadPoolDestroy(threadPool *pool);

void threadPoolAdd(threadPool *pool, void(*func)(void *), void *arg);
void threadExit(threadPool *pool);

int threadPoolBusyNum(threadPool* pool);

int threadPoolAliveNum(threadPool* pool);

int threadPoolTaskQueueSize(threadPool* pool);
void *managerThread(void *arg);

void *workThread(void *arg);












#endif
