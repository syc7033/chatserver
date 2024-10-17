#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
#include <string>

using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向redis指定的通道channel发布消息
    bool publish(int channel, string message);

    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    // 在独立的线程中接收订阅通道中的消息
    void observer_channel_message();

    // 初始化业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);

private:
    // hiredis同步上下文对象 负责publish消息 相当于一个redis客户端
    redisContext *_publish_context;

    // hiredis同步上下文对象 负责subcribe消息 相当于一个redis客户端
    redisContext *_subscribe_context;

    // 为什么需要两个hiredis上下文对象 因为使得subcribe会阻塞当前线程 使得publish无法进行

    // 回调操作 收到订阅消息 给service层上报
    function<void(int, string)> _notify_message_handler;
};