/*
muduo网络库给用户提供了两个主要的类
TcpServer和TcpClient，分别用于服务器和客户端的编程。

epoll + thread pool
好处： 能够将网络I/O代码和业务代码分离，提高并发性。

         业务：用户的连接和断开        用户的可读写事件
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;
/*
基于muduo网络库开发服务器程序
1. 组合TcpServer 对象
2. 创建EventLoop事件循环指针；
3. 明确TcpServer 构造函数需要什么参数，输出Chatserver的构造函数
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数；
5. 设置合适的服务端线程数量，muduo会自己分配IO线程和workr线程；
*/
class ChatServer
{
public:
        ChatServer(EventLoop* loop, //事件循环 Reactor
                const InetAddress& listenAddr, // ip+port
                const string& nameArg) // 服务器名字
                :_server(loop,listenAddr,nameArg),
                _loop(loop)
                {       
                        // 服务器注册用户连接的创建和断开回调
                        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
                        // 给服务器注册用户读写事件回调
                        _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
                        // 设置服务器端的线程数量 一个I/O线程 3个worker线程
                        _server.setThreadNum(4);
                }
        
        // 开启事件循环
        void start(){
                _server.start();
        }

private:   
        // 专门处理用户的连接创建和断开 epoll listenfd accept
        void onConnection(const TcpConnectionPtr& conn)
        {
                if(conn->connected())
                {
                        cout << conn->peerAddress().toIpPort() << "->" <<
                                conn->localAddress().toIpPort() << " state:online" << endl;     
                }else
                {
                     cout << conn->peerAddress().toIpPort() << "->" <<
                                conn->localAddress().toIpPort() << " state:offline" << endl;  

                        conn->shutdown(); // 调用用于关闭 TCP 连接
                        // _loop->quit(); 退出服务器    这是用于退出 事件循环 的方法  这通常意味着 关闭整个服务器 或者停止继续处理来自网络接口的事件。
                }
                
        }

        // 专门处理读写事件
        void onMessage(const TcpConnectionPtr& conn, //连接
                        Buffer* buffer, //缓冲区
                        Timestamp time //接收到数据的事件信息 
                        )
        {
                string buf = buffer->retrieveAllAsString();
                cout << "recv data " << buf << "time: " << time.toString() << endl;
                conn->send(buf);
        }


        TcpServer _server;
        EventLoop *_loop;  // epoll

};

int main()
{
        EventLoop loop; // epoll
        InetAddress addr("192.168.169.132", 8080);
        ChatServer server(&loop, addr, "Chatserver");

        server.start(); //listenfd 通过epoll_ctl->epoll 添加到epoll上面
        loop.loop(); // epoll_wait 以阻塞方式等待新用户连接，或者已连接用户的读写操作；
        return 0;
}