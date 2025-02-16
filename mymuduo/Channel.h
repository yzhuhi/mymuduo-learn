#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <memory>

#include <functional>

class EventLoop;
// class Timestamp;

/* 
    理清楚 EventLoop\ Channel\ poller之间的关系 
    Channel 理解为通道，封装了sockfd和其他感兴趣event,如EPOLLIN、EPOLLOUT事件
    还绑定了监听返回的具体事件；
*/
class Channel : noncopyable
{
public:

    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // fd得到poller通知后处理事件的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) {readCallback_ = std::move(cb);} // move左值转右值
    void setwriteCallback(EventCallback cb) { writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {  errorCallback_ = std::move(cb);}

    // 防止当Channel被手动remove掉，还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const{return fd_;}
    int events() const{return events_;}
    void  set_revents(int revt) { revents_ = revt; } // epoll监听事件，所以channel有一个对外接口让epoll设置到底什么事件发生
    

    // 设置fd相应的事件状态/*  */
    /* 
        外部通过这几个函数来告知Channel你所监管的文件描述符都对哪些事件类型感兴趣,并把这个文件描述符及其感
        兴趣事件注册到事件监听器(10多路复用模块)上。这些函数里面都有一个update()私有成员方法,这个update
        其实本质上就是调用了epoll ctl()。
     */
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() {events_ &= ~kReadEvent; update();}
    void enableWriting() {events_ |= kWriteEvent; update();}
    void disableWriting() {events_ &= ~kWriteEvent; update();}
    void disableAll() {events_ &= kNoneEvent; update();}

    // 返回fd当前的事件状态
    bool isNoneEvent() const{ return events_ == kNoneEvent;}
    bool isWriting() const{ return events_ & kWriteEvent;}
    bool isReading() const { return events_ & kReadEvent;}

    int index() { return index_;}
    void set_index(int idx) { index_ = idx;}

    // one loop per thread
    EventLoop* ownerLoop(){return loop_;}
    void remove();
private:
    
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_; // poller监听的对象
    int events_; // 注册fd感兴趣事件 epoll_ctl
    int revents_; // poller返回具体发生的事件
    int index_; 

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为Channl通道里面，能够获知fd最终发生的具体的事件revents，所以它负责具体事件的回调操作；
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};