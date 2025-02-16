#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

// 事件循环线程池类，负责管理多个事件循环线程
class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;  // 线程初始化回调函数类型

    // 构造函数，接收基事件循环和名称作为参数
    EventLoopThreadPool(EventLoop* baseloop, const std::string& nameArg);
    // 析构函数
    ~EventLoopThreadPool();

    // 设置线程数量
    void setThreadNum(int numThreads) { numThreads_ = numThreads;}

    // 开始线程池，启动多线程
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // 如果工作在多线程中，baseloop会默认以轮询方式分配channel个给subloop
    EventLoop* getNextLoop();

    // 获取所有的事件循环
    std::vector<EventLoop*> getAllLoops();

    // 判断是否已启动
    bool started() const {return started_;}
    // 获取线程池名称
    const std::string name() const { return name_;}

private:
    EventLoop* baseLoop_; // EventLop loop;
    std::string name_; // 线程池名称
    bool started_; // 是否已启动
    int numThreads_; // 线程数量
    int next_; // 下一个循环的索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 存储线程的唯一指针
    std::vector<EventLoop*> loops_; // 存储事件循环
};