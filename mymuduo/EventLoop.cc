#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop 加__thread表示这是一个thread_local 
__thread EventLoop* t_loopInThisThread = nullptr;

// 定义默认IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd, 用来notify唤醒sublReactor处理新来的Channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd <0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
    
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
    // , currentActivateChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %p \n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //每一个EventLoop都将监听wakeupchannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
  }
}

// 调用 EventLoop.loop() 正式开启事件循环，其内部会调用 Poller::poll -> ::epoll_wait正式等待活跃的事件发生，然后处理这些事件。
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start lopping \n", this);

    while(!quit_)
    {
        // 监听两类fd 一种是clientfd， 一种是wakeupfd(mainloop和subloop之间通信，用于唤醒subloop(也就是阻塞状态下的subloop))
        activateChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activateChannels_);
        for(Channel* channel: activateChannels_)
        {
            // Poller 监听哪些channel发生事件了，然后上报给EventLoop,通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /* 
            IO thread：mainLoop accept fd 打包成 chennel 分发给 subLoop
            mainLoop实现注册一个回调，交给subLoop来执行，wakeup subLoop 之后，让其执行注册的回调操作
            mainloop事先注册一个回调cb,(需要subloop执行)  wakeup subloop以后才唤醒 执行下面的方法，执行之前mainloop注册的回调操作
        */
        doPendingFunctors(); // 处理跨线程调用的回调函数
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环  1.loop在自己的线程中调用quit  2.在非loop的线程中，调用loop的quit
/* 
*                                    mainloop
            =================================生产者-消费在的线程安全队列（moduo库没有，而是直接通信，通过wakeupFd_进行直接的线程唤醒） 
    subloop1                        subloop2            subloop3          每一个loop里面对有一个wakeupFd_唤醒对应subloop
*/
void EventLoop::quit()
{
    quit_ = true;
    // 轮询算法派发mainloop建立的连接
    if(!isInLoopThread()) // 如果是在其他线程中调用quit， 在一个subloop（worker)中调用了mainloop(IO)的quit   --要先唤醒
    {
        wakeup();
    }
}

// 当前loop中执行
void EventLoop::runInloop(Functor cb)
{
    if(isInLoopThread()) // 在当前的loop线程中执行callback
    {
        cb();
    }
    else // 在非当前loop线程中执行callback ， 比如说在subloop2里面调用subloop3的 runInloop ,就需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}

// 把cb放入队列中， 唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        // // 操作任务队列需要保证互斥
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的， 需要执行上面回调操作的loop线程
    // 这里的callPendingFunctors_ 表示当前loop正在执行回调，但是loop又有了新回调
    
    if(!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); // 唤醒loop所在线程
    }
}


// 唤醒loop所在的线程    向wakeupfd_写一个数据,就能唤醒   wakeupChannel发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法调用Poller的方法 ,就是因为channel无法和poller直接沟通，通过eventloop来沟通，所以eventloop里面的函数底层还是调用poller的方法
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()  // 执行回调  相当于利用一个局部容器将应该执行回调的channel保存下来慢慢回调，这样就不会阻塞mainloop注册channel
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}