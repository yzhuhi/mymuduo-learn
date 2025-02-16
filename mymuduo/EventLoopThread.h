#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

class EventLoopThread : noncopyable
{
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;  // 定义线程初始化回调类型

        EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string& name = std::string()); // 构造函数，初始化回调和线程名称
        ~EventLoopThread();  // 析构函数

        EventLoop* startloop();  // 启动事件循环，并返回指向EventLoop的指针
    private:
        void threadFunc();  // 线程函数，负责运行事件循环

        EventLoop* loop_;  // 指向事件循环的指针
        bool exiting_;  // 标志，表示线程是否正在退出
        Thread thread_;  // 线程对象
        std::mutex mutex_;  // 互斥锁，用于保护共享数据
        std::condition_variable cond_;  // 条件变量，用于线程间通信
        ThreadInitCallback callback_;  // 存储线程初始化回调
};

/* 
EventLoopThread 类的主要功能是提供一个线程环境，以便在此线程中运行事件循环。该类允许用户定义线程初始化时要执行的特定逻辑，并通过其成员函数启动事件循环。
利用互斥锁和条件变量，类能够安全地处理多线程同步问题。整体上，这个类的设计有助于实现高效的事件驱动编程，并简化线程管理。
 */