#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include "errno.h"
#include "Buffer.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    // 如果传入EventLoop没有指向有意义的地址则出错
    // 正常来说在 TcpServer::start 这里就生成了新线程和对应的EventLoop
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
                const std::string& nameArg,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr_)
                : loop_(CheckLoopNotNull(loop))
                , name_(nameArg)
                , state_(kConnecting)
                , reading_(true)
                , socket_(new Socket(sockfd))
                , channel_(new Channel(loop,sockfd))
                , localAddr_(localAddr)
                , peerAddr_(peerAddr_)
                , highWaterMark_(64 * 1024 * 1024) // 64M 避免发送太快对方接受太慢
{
    // 下面给channel设置相应的回调函数，poller给channel通知感兴趣事件发生，channel回调相应的函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setwriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this)); //就是将Tcpconnction设置的回调绑定到channel上
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd = %d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG("TcpConnection::dtor[%s] at fd = %d state = %d\n", name_.c_str(), sockfd, int(state_));
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    size_t n = inputbuffer_.readFd(channel_->fd(), &saveErrno);
    if(n > 0)
    {
        // 已建立连接的用户有可读事件发生，调用用户传入的回调操作onMeaasge
        messageCallback_(shared_from_this(), &inputbuffer_, receiveTime);
    }
    else if (n==0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpCOnnection::handleRead");
        handleError();
    }
}



void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        int savedErrno = 0; 
        ssize_t n = outbuffer_.writeFd(channel_->fd(), &savedErrno);
        if(n>0)
        {
            outbuffer_.retrieve(n);
            if(outbuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)
                {
                    // 唤醒这个loop对应的thread线程
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else{
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else{
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d, state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行关闭连接的回调
    closeCallback_(connPtr); // 关闭连接的回调
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}

void TcpConnection::send(const std::string &buf)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInloop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
        }
    }
}

/* 
     * 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，而且设置了水位回调
*/
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该connction的shutdown，不能再进行发送了
    if(state_ == kDisconnecting)
    {
        LOG_ERROR("disconnected, give up writing");
        return;
    }
    //表示channel第一次开始写数据，而且缓冲区没有带发送数据
    if(!channel_->isWriting() && outbuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data,len);
        if(nwrote >= 0)
        {
            remaining = len - nwrote;
            if(remaining == 0&& writeCompleteCallback_)
            {
                //在这里一次性数据全部发送完成，就不用再给channel设置epollout事件了
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET) // 
                {
                    faultError = true;
                }
            }
        }
    }
    //说明当前一次write,并没有把数据全部发送出去，剩余数据需要保存到缓冲区当中，然后给channel
    // 注册pollout事件，poller发现tcp发送缓冲区有空间，会通知相应的channel调用handlwrite回调方法
    // 也就是调用Tcpconnection::handleWrite方法，把发送缓冲区中的和数据全部发送完成
    if(!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余待发送数据长度
        size_t olden = outbuffer_.readableBytes();
        if(olden + remaining >= highWaterMark_ && olden < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(),olden+remaining));
        }
        outbuffer_.append((char*)data + nwrote, remaining);
        if(!channel_->isWriting())
        {
            channel_->enableWriting(); // 这里一定要注册channel的写事件，否则poller不会给channel通知epollout
        }
    }

}

// 连接建立  TcpConnection直接给到用户手里，所以需要检查
void TcpConnection::connectionEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this()); // 
    channel_->enableReading(); // 向poller注册Channel的epollin事件（读）

    // 新连接建立，执行回调
    connectionCallback_(shared_from_this()); // 相当于this->connectionCallback_()安全变体
}
//连接销毁
void TcpConnection::connectDestoryed()
{
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel所有感兴趣事件，从poller中del
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除
}


// 关闭连接
void TcpConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInloop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()) // 说明outbuffer中的数据已经被全部发送完成；
    {
        socket_->shutdownWrite(); // 关闭写端
    }
}



