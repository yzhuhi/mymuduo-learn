#include "InetAddress.h"
#include <iostream>
#include <strings.h>
#include <string.h>


InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_); //使用 bzero 函数清空 addr_，确保没有残留数据。
    addr_.sin_family = AF_INET; // 设置地址族为 IPv4（AF_INET）。
    addr_.sin_port = htons(port); // 使用 htons 函数将端口号转换为网络字节序，确保跨平台的一致性。
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // 将输入的字符串类型 IP 地址转换为网络字节序，并赋值给 sin_addr.s_addr。
}

std::string InetAddress::toIP() const
{
    // addr_
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf)); // 使用 inet_ntop 函数将网络字节序的 IP 地址转换为可读的字符串格式，并存储在 buf 中，最后返回该字符串。
    return buf;
}

std::string InetAddress::toIpPort() const  // 将 IP 地址和端口号组合成一个字符串，格式为 "ip:port"。
{
    // ip::port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port); // 这一行代码的作用是将端口号格式化为字符串并追加到已有的 IP 地址字符串后面。
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port); //使用 ntohs 函数将端口号从网络字节序转换为主机字节序并返回。
}



// int main()
// {
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort()<< std::endl;
//     return 0;
// }
