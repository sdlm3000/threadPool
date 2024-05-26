#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "threadpool.h"

void taskFunc(void* arg)
{
    int num = *(int*)arg;
    printf("thread %ld is working, number = %d\n",
        pthread_self(), num);
    usleep(1000000);
}

int main(void) 
{
    // threadPool *myThreadPool;
    // myThreadPool = threadPoolCreate(3, 20, 6);
    // int x = 1, y =2;
    // threadPoolAdd(myThreadPool, taskFunc, &x);
    // threadPoolAdd(myThreadPool, taskFunc, &y);
    // threadPoolAdd(myThreadPool, taskFunc, &x);
    // threadPoolAdd(myThreadPool, taskFunc, &y);
    // sleep(5);
    // threadPoolDestroy(myThreadPool);
    // 创建线程池
    threadPool* pool = threadPoolCreate(3, 20, 100);
    for (int i = 0; i < 40; ++i)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 100;
        threadPoolAdd(pool, taskFunc, num);
    }
    printf("sleep\n");
    for(int i = 0; i < 20; i++) {
        int aliveNum = threadPoolAliveNum(pool);
        int busyNum = threadPoolBusyNum(pool);
        int queueSize = threadPoolTaskQueueSize(pool);
        printf("\n************** aliveNum = %d, busyNum = %d, queueSize = %d\n", 
                aliveNum, busyNum, queueSize);
        sleep(1);
    }


    threadPoolDestroy(pool);
    return 0;

    return 0;
}
