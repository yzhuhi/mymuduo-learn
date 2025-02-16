#pragma once

#include "Poller.h"
#include <vector>
#include <sys/epoll.h>

class Channel;

/* 
    epoll使用
    epoll_create
    epoll_ctl  add/mod/del
    epoll_wait
 */

class EPollPoller : public Poller  // 封装epoll的主要操作行为
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;  // override表示让编译器去检查是覆盖函数也就是基类里面是虚函数；

    // 重写基类的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activateChannels) override;
    void updateChannel(Channel* channel) override; // 对应epoll_ctl行为
    void removeChannel(Channel* channel) override;
private:
    // 默认监听事件数量  当调用 epoll_wait() 时，这个数组会存储当前被触发的事件。如果超过了初始大小（16 个），会动态扩展这个数组。
    // 表示初始监听事件缓冲区大小为 16，但不是 epoll 的硬限制。
    static const int kInitEventListSize = 16;

    // 填写活跃的链接
    void fillActiveChannels(int numEvents, ChannelList* activateChannels) const;
    // 更新channel通道，本质是调用了epoll_ctl
    void update(int operation, Channel* channel);

    using EventList = std::vector<epoll_event>;
     
    int epollfd_; // 对应epoll_create()创建的文件系统  epoll_create 创建的指向 epoll 对象的文件描述符（句柄）
    EventList events_; //返回事件的数组 

};