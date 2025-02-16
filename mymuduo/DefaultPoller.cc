#include "Poller.h"
#include "EPollPoller.h"
#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生成poll的实例
    }
    else
    {
        return new EPollPoller(loop); // 生成epoll的实例
    }
}


// 公共源文件， 但是poller属于最上层，派生类可以引用他，但是他最好不要去引用派生类 ------好的思想