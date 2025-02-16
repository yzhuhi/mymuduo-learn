#pragma once

/*
    noncopyable被继承后，派生类对象可以正常构造和析沟，但是派生类对象无法进行拷贝构造和赋值操作；
*/
class noncopyable
{
    public:
        noncopyable(const noncopyable&) = delete;
        noncopyable& operator=(const noncopyable&) = delete;
    private:
    protected:
        noncopyable() = default;
        ~noncopyable() = default;
};