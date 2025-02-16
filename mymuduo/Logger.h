#pragma once

// 定义日志级别 INFO ---- ERROR ---- FATAL ----DEBUG

#include "noncopyable.h"
#include <string>

// LOG_INFO("%s, %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif


enum LogLevel
{
    INFO, // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG // 调试信息
};

// 一个日志类
class Logger :  noncopyable
{
    public:
        // 获取唯一的实例对象；
        static Logger& instance();
        // 设置日至级别
        void setLogLevel(int level);
        // 写日志
        void log(std::string msg);
    private:
        int loglevel_; // 和系统变量不产生冲突，一般是前_
        Logger(){}  // 私有构造函数，确保只能通过 instance() 获取单例
};