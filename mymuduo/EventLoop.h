#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
// 事件循环类，可以看作reactor和事件分发器，主要包含了Channel 和 Poll（epoll的抽象）一个epoll对应一堆channel


class Channel;
class Poller;


class EventLoop :noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    // 当前loop中执行
    void runInloop(Functor cb);
    // 把cb放入队列中， 唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在的线程
    void wakeup();
    
    // EventLoop的方法调用Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}

private:
    void handleRead(); // wakeup
    void doPendingFunctors(); //执行回调
    
    using ChannelList = std::vector<Channel* >;

    std::atomic_bool looping_; // 原子操作，通过CAS实现，标志正在执行事件循环
    std::atomic_bool quit_; // 标识退出loop循环 标志退出事件循环
    
    const pid_t threadId_; // 记录当前loop所在线程的id

    Timestamp pollReturnTime_; // poller返回发生事件的channels的返回时间
    std::unique_ptr<Poller> poller_;

    /* 
     * TODO:eventfd用于线程通知机制，libevent和我的webserver是使用sockepair
     * 作用：当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 
     * 通过该成员唤醒subLoop处理Channel
    */
    int wakeupFd_;   // mainLoop向subLoop::wakeupFd写数据唤醒
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activateChannels_; // 活跃的Channel，poll函数会填满这个容器
    // Channel* currentActivateChannel_;  // 当前处理的活跃channel

    std::atomic_bool callingPendingFunctors_; //标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop跨线程需要执行的所有回调操作
    std::mutex mutex_; //互斥锁用来保护上面vector容器的线程安全操作
};

/* 
wakeupFd_：如果需要唤醒某个EventLoop执行异步操作，就向其wakeupFd_写入数据。
activeChannels_：调用poller_->poll时会得到发生了事件的Channel，会将其储存到activeChannels_中。
pendingFunctors_：如果涉及跨线程调用函数时，会将函数储存到pendingFunctors_这个任务队列中。
*/