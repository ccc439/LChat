#ifndef SINGLETON_H
#define SINGLETON_H
#include "global.h"

//单例模式，基类
template<typename T>
class Singleton{
public:
    static std::shared_ptr<T> getInstance(){//返回单例智能指针
        static T _instance;
        return std::shared_ptr<T>(&_instance, [](T*){});
    }
protected://设为protected是为了让子类能调用
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};


#endif // SINGLETON_H
