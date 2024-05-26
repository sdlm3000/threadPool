#include <queue>
#include <mutex>

using callback = void(*)(void *);

class Task 
{
public:
    Task() 
    {
        function = nullptr;
        arg = nullptr;
    }

    Task(callback func, void *arg) 
    {
        this->function = func;
        this->arg = arg;
    }

public:
    callback function;
    void* arg;
};


// 任务队列
class taskQueue
{
public:
    taskQueue();
    ~taskQueue();

    // 添加任务
    void addTask(Task &task);
    void addTask(callback func, void *arg);

    // 取出一个任务
    Task takeTask();

    // 获取任务数量
    int getTaskNumber();

private:
    std::mutex m_mutex;
    std::queue<Task> m_queue;

};

