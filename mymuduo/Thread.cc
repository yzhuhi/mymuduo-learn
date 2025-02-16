#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

// 使用std::atomic_int声明一个静态原子变量numCreated_，用来安全地记录创建的线程数量。
std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    :started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(started_ &&  !joined_)
    {
        thread_->detach();
    }
}

/* 
创建一个新线程以执行指定的线程函数。
在新线程中获取该线程的ID，并将其存储在对象的成员变量中。
主线程（调用start()的线程）会在sem_wait(&sem)处等待，直到新线程成功获取到它的线程ID并调用sem_post(&sem)，从而释放信号量。
这意味着start()方法在获取到新线程的ID之前不会返回，确保了在主线程继续执行之前，新线程的信息已经被完全设置好。这样可以有效避免在尚未准备好的情况下访问线程ID，确保线程安全性。
 */
void Thread::start() // 一个thread对象记录的就是一个新线程的详细信息；
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程tid
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_(); // 开启一个新线程，专门执行该线程函数
    }));

    // 这里必须等待获取上面新创建线程的tid
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}


void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}