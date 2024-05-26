#include <iostream>
#include "threadPool.h"
#include <unistd.h>

void testFun(void *arg) {
    int num  = *(int *)arg;
    std::cout << "this a " << num << " thread: " << std::this_thread::get_id() << std::endl;
    sleep(1);
}

int main() {
    threadPool *pool = new threadPool(3, 10);
    for(int i = 0; i < 50; i++) {
        int *num  = new int(i);
        pool->addTask(Task(testFun, num));
    }
    printf("**********************  sleep  ***********************\n");
    for(int i = 0; i < 30; i++) {
        int aliveNum = pool->getAliveNumber();
        int busyNum = pool->getBusyNumber();
        int queueSize = pool->getTaskNumber();
        printf("\n************** aliveNum = %d, busyNum = %d, queueSize = %d\n", 
                aliveNum, busyNum, queueSize);
        sleep(1);
    }
    return 0;
}