#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>
#include <functional>
#include "Logger.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;


// EventLoop: ChannelList Poller
Channel::Channel(EventLoop* loop, int fd) 
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{

}

Channel::~Channel()
{
//     if (loop_->isInLoopThread())
//     {
//         assert(!loop_->hasChannel(this));
//     }  //这个操作就是判断当前这个loop是否是在这个线程下面，以及当前这个channel是否实在这个loop下面，因为目前都是高并发服务器，多线程

}

// channel的tie方法什么时候调用？一个Tcpconnection新连接建立的时候 TcpConnection => channel
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj; // 仅将 weak_ptr 指向 shared_ptr 管理的对象
    tied_ = true; // 标记已经使用了 tie_ 机制
}

// 该方法的作用是，当改变channel所表示的fd的事件后，update负责在poller里面更改fd相应的事件 epoll_ctl
// EventLoop => ChannelList       Poller     独立
void Channel::update()
{
    // 通过Channel所属的EventLoop调用poller的相应方法，注册fd的event事件；
    // add code...
    loop_->updateChannel(this);

}

// 在Channel所属的EventLoop中，把当前的channel删除
void Channel::remove()
{   
    // add code ..
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    } else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件，由channel负责调用具体回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    if(revents_ && EPOLLHUP && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }

    if(revents_ & (EPOLLERR))
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}
