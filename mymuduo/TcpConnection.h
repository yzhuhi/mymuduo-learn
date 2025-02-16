#pragma once
#include "noncopyable.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "Timestamp.h"
#include"Socket.h"

#include <memory>
#include <string>
#include <atomic>


class Channel; // 用户给TcpServer-->TcpConnection --->Channel
class EventLoop;
class Scoket;


/* 
*   TcpServer == 》 Acceptor => 有一个新用户连接，通过accept函数拿到connfd;

    打包TcpConnection 设置回调 =》 Channel =>Poller => Channel的回调；
*/

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr_);
    ~TcpConnection();

    EventLoop* getloop() const { return loop_;}
    const std::string& name() const { return name_;}
    const InetAddress& localAddress() const {return localAddr_;}
    const InetAddress& peerAddress() const {return peerAddr_;}

    bool connected() const { return state_ == kConnected;}

    // 发送数据
    // void send(const void* message, int len);
    void send(const std::string& buf);
    void send(Buffer *buf);

    // 关闭连接
    void shutdown();

    // TcpConnection设置回调 会传给Channel     保存用户自定义的回调函数
    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb)
    {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        writeCompleteCallback_ = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback& cb)
    {
        closeCallback_ = cb;
    }

    // TcpServer会调用
    // 连接建立
    void connectionEstablished();
    //连接销毁
    void connectDestoryed();


    
private:
    enum StateE
    {
        kDisconnected, // 已经断开连接
        kConnecting,   // 正在连接
        kConnected,    // 已连接
        kDisconnecting // 正在断开连接
    };    

    void setState(StateE state) { state_ = state;}

    // 注册到channel上的回调函数，poller通知后会调用这些函数处理
    // 然后这些函数最后会再调用从用户那里传来的回调函数，所以TcpConnection是用户可以直接使用的接口，通过用户自己给的回调函数
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();

    
    // 这里绝对不是baseloop,因为TcpConnection都是在subloop里面管理的，
    //  所以该Tcp连接的Channel注册到了哪一个subloop,这个loop就对应
    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_; // Tcp连接状态
    bool reading_;

    // 这里和Acceptor类似       Acceptor 在mainloop       TcpConnection 在subloop  相似
    std::unique_ptr<Socket> socket_; // 就是Acceptor注册的那个回调函数创建出来的connfd（新连接fd)
    std::unique_ptr<Channel> channel_; // 封装了上面的socket_及保存的给类事件的处理函数，都是在TcpConnection这个类的实例化对象构造函数注册的


    const InetAddress localAddr_; // 本服务器地址
    const InetAddress peerAddr_; // 对端地址

    ConnectionCallback connections_; // 保存所有的连接

    /**
     * 用户自定义的这些事件的处理函数，然后传递给 TcpServer 
     * TcpServer 再在创建 TcpConnection 对象时候设置这些回调函数到 TcpConnection中
     */
    ConnectionCallback connectionCallback_;         // 有新连接时的回调
    MessageCallback messageCallback_;               // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_;   // 超出水位实现的回调
    CloseCallback closeCallback_;                   // 客户端关闭连接的回调

    size_t  highWaterMark_;

    Buffer inputbuffer_; // Tcp连接 对应的用户接收数据缓冲区
    Buffer outbuffer_; 
    // 暂存那些暂时发不出去的待发送数据。因为Tcp发送缓冲区有大小限制的，加入到了高水位线就没办法把发送的数据通过send()直接拷贝到Tcp发送缓冲区
    // ，就暂存在这个outbuffer中，等Tcp发送缓冲区有空间了，触发可读事件，再把buffer里面的数据拷贝到发送缓冲区

};