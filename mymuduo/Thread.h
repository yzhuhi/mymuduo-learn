#pragma once

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>; // 定义线程函数类型

    explicit Thread(ThreadFunc, const std::string& name = std::string()); // 构造函数，初始化线程
    ~Thread(); // 析构函数，清理资源

    void start(); // 启动线程
    void join(); // 等待线程结束

    bool started() const{return started_;} // 检查线程是否已启动
    pid_t tid() const{return tid_;} // 获取线程ID
    const std::string& name() const { return name_;} // 获取线程名称

    static int numCreated() {return numCreated_;} // 获取创建的线程数量
private:
    void setDefaultName(); // 设置默认线程名称

    bool started_; // 线程启动状态
    bool joined_; // 线程连接状态

    std::shared_ptr<std::thread> thread_; // 用于线程管理的智能指针
    pid_t tid_; // 线程ID
    ThreadFunc func_; // 线程要执行的函数
    std::string name_; // 线程名称
    static std::atomic_int numCreated_; // 创建的线程数量
};