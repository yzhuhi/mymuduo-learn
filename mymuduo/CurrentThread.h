#pragma once
#include <unistd.h>
#include <sys/syscall.h>

/* 
    首次调用时的行为：

    当线程第一次调用 tid() 时，t_cachedTid 的初始值为 0。
    tid() 内部会调用 cachedTid()，通过 syscall(SYS_gettid) 获取线程 ID，并将其缓存到 t_cachedTid。
    这次调用确实会触发一次 系统调用（用户态到内核态的开销）。
    后续调用时的行为：

    之后的每次调用 tid() 时，t_cachedTid 已经被设置为线程 ID，且线程 ID 不会改变。
    tid() 函数直接返回 缓存的 t_cachedTid，不会再次调用 syscall。
    这意味着 后续调用不会再触发系统调用，仅在用户态完成，代价很小。
*/
namespace CurrentThread
{
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}