#pragma once

#include <mymuduo/TcpServer.h>

// 自定义的服务器类
class ChatServer
{
public:
    // 在自定义服务器中初始化muduo库的TcpServer
    ChatServer(EventLoop *loop,
            const InetAddress &listenAddr,
            const std::string &nameArg);
    void start();
private:
    // baseLoop 主事件循环
    EventLoop *loop_;
    // muduo库的TcpServer
    TcpServer server_;
    // 当有用户连接的时, 会调用该回调函数
    void onConnection(const TcpConnectionPtr&);
    // 当用户读写事件发生 会调用该回调函数
    void onMessage(const TcpConnectionPtr&, Buffer*, Timestamp);
};