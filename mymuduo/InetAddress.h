#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装socket地址类型
class InetAddress
{
    public:
        // 构造函数，使用端口和IP地址初始化InetAddress对象，默认IP为127.0.0.1
        explicit InetAddress(uint16_t port=0, std::string ip="127.0.0.1");
        
        // 构造函数，使用sockaddr_in结构体初始化InetAddress对象
        explicit InetAddress(const sockaddr_in& addr)
            : addr_(addr){}
        
        // 返回IP地址的字符串表示
        std::string toIP() const;
        
        // 返回IP地址和端口号的字符串表示
        std::string toIpPort() const;

        // 返回端口号
        uint16_t toPort() const;

        // 返回sockaddr_in结构体的指针
        const sockaddr_in* getSockAddr() const{return &addr_;}
        
        // 
        void setSockAddr(const sockaddr_in& addr) { addr_ = addr;}
        
    private:
        // 存储sockaddr_in地址信息
        sockaddr_in addr_;
};