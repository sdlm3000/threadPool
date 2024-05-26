#include "taskQueue.h"

taskQueue::taskQueue()
{
}
taskQueue::~taskQueue()
{
}

// 添加任务
void taskQueue::addTask(Task &task)
{
    m_mutex.lock();
    m_queue.push(task);
    m_mutex.unlock();
}

void taskQueue::addTask(callback func, void *arg)
{
    m_mutex.lock();
    m_queue.push(Task(func, arg));
    m_mutex.unlock();
}

// 取出一个任务
Task taskQueue::takeTask()
{
    Task task;
    m_mutex.lock();
    if(m_queue.size() > 0) {
        task = m_queue.front();
        m_queue.pop();
    }
    m_mutex.unlock();
    return task;
}


int taskQueue::getTaskNumber()
{
    int size;
    m_mutex.lock();
    size = m_queue.size();
    m_mutex.unlock();
    return size;
}