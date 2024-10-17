#include "chatserver.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <signal.h>

using namespace std;

// 处理服务器ctrl + c结束后 重置user的状态信息
void resetHandler(int)
{
    ChatService::getInstance().reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    //  解析命令行传递过来的ip:port
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT, resetHandler);
    
    EventLoop loop;
    InetAddress addr(port, ip);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop(); // 开启baseLoop的事件循环 监听用户的连接
    
    return 0;
}