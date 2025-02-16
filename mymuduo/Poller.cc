#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop) : ownerLoop_(loop)
{

}

bool Poller::hasChannel(Channel* channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}


// 默认IO复用 机制 Eventloop事件循环可以通过该接口获取默认的IO复用具体实现
    /* 
        // 如果放到cc文件中实现，那不可避免会include PollerPoller.h  EpollPoller.h 
        不合理，派生类可以引用基类，但是基类不可以引用派生类
        所以这里不能用Poller基类去引用派生类
    */
    // static Poller* newDefaultPoller(EventLoop* loop); 