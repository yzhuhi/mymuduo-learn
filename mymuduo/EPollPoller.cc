#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <memory>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

// channel未添加到poller中
const int kNew = -1; // channel成员index初始化-1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2; //channel已经从poll删除

// epoll_create()
EPollPoller::EPollPoller(EventLoop* loop) 
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) // vector<epoll_event>作为一个线性存储，最终会把发生事件的fd的event放到这个里面 
{
    if(epollfd_< 0)
    {
        LOG_FATAL("epoll_create error: %d \n", errno);
    }
}

EPollPoller::~EPollPoller() 
{
    ::close(epollfd_);
}

// epoll_ctl
// channel update remove （实质是调用）=> EventLoop updateChanne removeChannel => Poller updateChanne removeChannel
/* 
            EventLoop
    ChannelList         Poller
                    >=    ChannelMap <fd, channel*>  // 注册过的
*/
void EPollPoller::updateChannel(Channel* channel) 
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel已经在poll注册过了
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中移除channel的逻辑
void EPollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func= %s => fd=%d events=%d \n", __FUNCTION__, channel->fd(), channel->events());


    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);

}

// 更新channel通道， epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel* channel)
{
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;
    

    if(::epoll_ctl(epollfd_,operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll ctl add/mod error:%d\n", errno);
        }
    }
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activateChannels) const
{
    for(int i=0; i< numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activateChannels->push_back(channel); //EventLoop就拿到了它的poller给他返回的所有发生时间的
    }
}



// epoll_wait() // 核心函数，其内部不断调用epoll_wait获取发生事件

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activateChannels)
{
    // 实际上应该用LOG_DEBUG输出日志更合理
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activateChannels);
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }

    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout \n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err");
        }
    }
    return now;

}