
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "threadpool.h"

#define VAR_NUMBER      2

threadPool *threadPoolCreate(int minNum, int maxNum, int queueCapacity)
{
    threadPool *pool = (threadPool *)malloc(sizeof(threadPool));
    if(pool == NULL) {
        printf("malloc threadpool fail!\n");
        goto error;
    }
    pool->maxNum = maxNum;
    pool->minNum = minNum;
    pool->aliveNum = minNum;
    pool->busyNum = 0;
    pool->exitNum = 0;

    // 初始化互斥锁和条件变量
    if(pthread_mutex_init(&(pool->mutexPool), NULL) != 0) {
        printf("mutexPool init fail!\n");
        goto error;
    }
    if(pthread_mutex_init(&(pool->mutexBusy), NULL) != 0) {
        printf("mutexBusy init fail!\n");
        goto error;
    }
    if(pthread_cond_init(&(pool->taskNotFull), NULL) != 0) {
        printf("taskNotFull init fail!\n");
        goto error;
    }
    if(pthread_cond_init(&(pool->taskNotEmpty), NULL) != 0) {
        printf("taskNotEmpty init fail!\n");
        goto error;
    }

    // 任务队列
    pool->taskQueue = (task *)malloc(sizeof(task) * queueCapacity);
    pool->queueCapacity = queueCapacity;
    pool->queueSize = 0;
    pool->queueFront = 0;
    pool->queueTail = 0;
    pool->shutdown = 0;

    // 创建管理线程
    int ret = pthread_create(&(pool->managerID), NULL, managerThread, pool);
    if(ret != 0) {
        printf("create manager thread fail %d\n", ret);
        goto error;
    }


    // 创建工作线程
    pool->workIDs = (pthread_t *)malloc(sizeof(pthread_t) * pool->maxNum);
    if (pool->workIDs == NULL)
    {
        printf("malloc threadIDs fail...\n");
        goto error;
    }
    memset(pool->workIDs, 0, sizeof(pthread_t) * maxNum);
    for(int i = 0; i < pool->aliveNum; i++) {
        int ret = pthread_create(&(pool->workIDs[i]), NULL, workThread, pool);
        if(ret != 0) {
            printf("create %d work thread fail %d\n", i, ret);
            goto error;
        }
        pthread_detach(pool->workIDs[i]);
    }

    return pool;
error:
    if(pool && pool->workIDs) {
        free(pool->workIDs);
        pool->workIDs = NULL;
    }
    if(pool && pool->taskQueue) {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }
    if(pool) {
        free(pool);
        pool = NULL;
    }

    return NULL;


}


int threadPoolDestroy(threadPool *pool)
{
    if(pool == NULL) {
        return -1;
    }
    pthread_mutex_lock(&pool->mutexPool);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutexPool);
    pthread_join(pool->managerID, NULL);
    for(int i = 0; i < pool->aliveNum; i++) {
        pthread_cond_signal(&pool->taskNotEmpty);
    }

    if(pool->taskQueue != NULL) {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }
    if(pool->workIDs != NULL) {
        free(pool->workIDs);
        pool->workIDs = NULL;
    }

    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->taskNotEmpty);
    pthread_cond_destroy(&pool->taskNotFull);

    free(pool);
    pool = NULL;

    return 0;


}

int threadPoolBusyNum(threadPool* pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);
    return busyNum;
}

int threadPoolAliveNum(threadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int aliveNum = pool->aliveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return aliveNum;
}

int threadPoolTaskQueueSize(threadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int taskQueueSize = pool->queueSize;
    pthread_mutex_unlock(&pool->mutexPool);
    return taskQueueSize;
}

void threadPoolAdd(threadPool *pool, void(*func)(void*), void *arg)
{
    pthread_mutex_lock(&pool->mutexPool);
    while(pool->queueSize >= pool->queueCapacity) {
        pthread_cond_wait(&pool->taskNotFull, &pool->mutexPool);
    }
    pool->taskQueue[pool->queueTail].function = func;
    pool->taskQueue[pool->queueTail].arg = arg;
    pool->queueTail = (pool->queueTail + 1) % pool->queueCapacity;
    pool->queueSize++;
    pthread_cond_signal(&pool->taskNotEmpty);
    pthread_mutex_unlock(&pool->mutexPool);
}

void threadExit(threadPool *pool)
{
    pthread_t selfId = pthread_self();
    for(int i = 0; i < pool->maxNum; i++) {
        if(pool->workIDs[i] == selfId) {
            pool->workIDs[i] = 0;
            printf("threadExit() called, %ld exiting...\n", selfId);
            break;
        }
    }
    pthread_exit(NULL);
}

void *managerThread(void *arg)
{
    threadPool *pool = (threadPool *)arg;
    while(1) {
        // 线程池是否退出
        pthread_mutex_lock(&pool->mutexPool);
        int shutdown = pool->shutdown;
        pthread_mutex_unlock(&pool->mutexPool);
        if(shutdown == 1) {
            pthread_t selfId = pthread_self();
            printf("manager thread %ld exit...\n", selfId);
            break;
        } 
        // 每3秒检查一次
        sleep(3);
        pthread_mutex_lock(&pool->mutexPool);
        int aliveNum = pool->aliveNum;
        int busyNum = pool->busyNum;
        int queueSize = pool->queueSize;
        pthread_mutex_unlock(&pool->mutexPool);

        // 检查是否增加线程
        if(queueSize > aliveNum && aliveNum < pool->maxNum) {
            for(int i = 0, cnt = 0; i < pool->maxNum && cnt < VAR_NUMBER &&
                    pool->aliveNum < pool->maxNum; i++) {
                if(pool->workIDs[i] == 0) {
                    int ret = pthread_create(&(pool->workIDs[i]), NULL, workThread, pool);
                    if(ret != 0) {
                        printf("Add %d work thread fail %d\n", i, ret);
                        break;
                    }
                    pthread_detach(pool->workIDs[i]);
                    cnt++;
                    pthread_mutex_lock(&pool->mutexPool);
                    pool->aliveNum++;
                    pthread_mutex_unlock(&pool->mutexPool);
                }
            }   
        }
        // 检查是否减少线程
        if(busyNum * 2 < aliveNum && aliveNum > pool->minNum) {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = pool->exitNum + VAR_NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            for(int i = 0; i < VAR_NUMBER; i++) {
                pthread_cond_signal(&pool->taskNotEmpty);
            }
 
        }
    }
}

void *workThread(void *arg)
{
    threadPool *pool = (threadPool *)arg;
    while(1) {
        // 等待任务队列不为空
        pthread_mutex_lock(&pool->mutexPool);
        // while防止虚假唤醒
        while(pool->queueSize == 0 && pool->shutdown == 0) {
            pthread_cond_wait(&pool->taskNotEmpty, &pool->mutexPool);
            // 被唤醒后，exitNum大于0，则退出该线程
            if(pool->exitNum > 0) {
                pool->exitNum--;
                if(pool->aliveNum > pool->minNum) pool->aliveNum--;
                pthread_mutex_unlock(&pool->mutexPool);
                threadExit(pool);
            }
        }
        if(pool->shutdown == 1) {
            pthread_mutex_unlock(&pool->mutexPool);
            threadExit(pool);
            break;
        }
        // 取出一个任务
        task *workTask = &(pool->taskQueue[pool->queueFront]);
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        pthread_cond_signal(&pool->taskNotFull);    // 给taskNotFull发送信号
        pthread_mutex_unlock(&pool->mutexPool);
        
        printf("thread %ld start working...\n", pthread_self());
        // 执行任务，并对busyNum进行操作
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        workTask->function(workTask->arg);

        free(workTask->arg);
        workTask->arg = NULL;

        printf("thread %ld end working...\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);

    }
}


