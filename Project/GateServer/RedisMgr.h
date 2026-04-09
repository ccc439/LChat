#pragma once
#include "const.h"

//Redis连接池
class RedisConPool {
public:
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd);
    RedisConPool() = default;
    ~RedisConPool();
    redisContext* getConnection();//获取连接
    void returnConnection(redisContext* context);//返还连接
    void Close();//关闭连接池
private:
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    const char* host_;
    int port_;
    std::queue<redisContext*> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

class RedisMgr:public Singleton<RedisMgr>,public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    bool Get(const std::string& key, std::string& value);//获取key对应的value
    bool Set(const std::string& key, const std::string& value);//设置key和value
    bool Auth(const std::string& password);//密码认证
    bool LPush(const std::string& key, const std::string& value);//左Push
    bool LPop(const std::string& key, std::string& value);//左Pop
    bool RPush(const std::string& key, const std::string& value);//右Push
    bool RPop(const std::string& key, std::string& value);//右Pop
    //二级缓存Map
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);//一级key，二级key，value
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);//一级key，二级key，value，value长度
    std::string HGet(const std::string& key, const std::string& hkey);
    bool Del(const std::string& key);//删除key及其对应的值
    bool ExistsKey(const std::string& key);//判断key是否存在
    void Close();//关闭连接
private:
    RedisMgr();
    std::unique_ptr<RedisConPool> con_pool_;
};
