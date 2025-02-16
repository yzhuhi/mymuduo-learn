#include "Logger.h"
#include <iostream>
#include "Timestamp.h"

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

// 设置日至级别
void Logger::setLogLevel(int level)
{
    loglevel_ = level;
}

// 写日志 [级别信息] time : msg
void Logger::log(std::string msg)
{
    switch (loglevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    
    default:
        break;
    }

    // print time and msg
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}