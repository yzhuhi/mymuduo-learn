#pragma once

#include <vector>
#include <string>
#include <algorithm>

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size


// 网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
        {

        }
    
    /**
     * kCheapPrepend | reader | writer |
     * writerIndex_ - readerIndex_
    */
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    /**
     * kCheapPrepend | reader | writer |
     * buffer_.size() - writerIndex_
     */   
    size_t writableBtes() const
    {
        return buffer_.size() - writerIndex_; 
    }

    /**
     * kCheapPrepend | reader | writer |
     * readerIndex_
     */ 
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // 获取剩余的所有
    // void retrieveUntil(const char *end)
    // {
    //     retrieve(end - peek());
    // }

    // onMessage string <- Buffer
    // 需要进行复位操作
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len; // 说明应用只读取了可读缓冲区的一部分，就是len，还剩下readerIndex_ += len --->writeIndex_
        }
        else{
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的buffer数据转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 应用可读取数据长度
    }

    std::string retrieveAsString(size_t len)
    {
        // peek()可读数据的起始地址
        std::string result(peek(), len);
        retrieve(len); // 上面一句把缓冲区中可读取的数据读取出来，所以要将缓冲区复位
        return result;
    }

    // buffer_.size() - writerIndex_   len
    void ensureWriteableBytes(size_t len)
    {
        if(writableBtes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }


    // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据  也就是缓冲区
    ssize_t readFd(int fd, int* saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);
    
private:
    char* begin()
    {
        // it.operator*() .operator&()
        return &*buffer_.begin(); // vector底层数组首元素地址，也就是数组的起始地址
    }

    const char* begin() const
    {
        return &(*buffer_.begin());
    }

    void makeSpace(size_t len)
    {
        if(writableBtes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};