#include "CurrentThread.h"
#include <unistd.h>
#include <sys/syscall.h>

/* 
    syscall(SYS_gettid)：直接使用系统调用获取当前线程的 TID（Linux）。
    SYS_gettid 是系统调用号，用于获取线程 ID。
    ::syscall 是系统调用接口，返回值是线程 ID（pid_t 类型）。
    作用：当 t_cachedTid 为 0 时，使用系统调用获取线程 ID 并缓存到 t_cachedTid 中。
*/
namespace CurrentThread
{
    __thread int t_cachedTid = 0;   

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            // 通过linux系统调用，获取当前线程的tid值
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}