#pragma once

#include "noncopyable.h"
#include <vector>
#include <unordered_map>
#include "Timestamp.h"
class Channel;
class EventLoop;

// muduo库中多路事件分发器 Poller 实际上是可以是在poll\epoll\select上 封装的 不写死
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default; // must be called in the loop thread

    // 给所有IO复用保留统一接口
    virtual Timestamp poll(int timeout, ChannelList* activateChannel) = 0; // pure virtual
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // 判断参数channel是否在当前poller当中
    bool hasChannel(Channel* channel) const;

    // 默认IO复用 机制 Eventloop事件循环可以通过该接口获取默认的IO复用具体实现
    /* 
        // 如果放到cc文件中实现，那不可避免会include PollerPoller.h  EpollPoller.h 
        不合理，派生类可以引用基类，但是基类不可以引用派生类
    */
    static Poller* newDefaultPoller(EventLoop* loop); 
protected:
    // map的          key：sockfd, val:sockfd所属通道类型   通过fd快速查找channel
    using ChannelMap = std::unordered_map<int, Channel* >;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_; //定义Poller所属的事件循环EventLoop
};