/* 
muduo网络库给用户提供了两个主要的类
    TcpServer：用于编写服务器程序的
    TcpClient：用于编写客户端程序的

epoll + 线程池

好处：能够把 网络I/O 的代码和 业务 代码区分开
我们的程序员只需要关注业务代码的部分
业务方面主要感兴趣的就两个点：
        a.用户的建立连接事件和断开连接事件
        b.用户的可读写事件
        只需要关注这俩件事怎么做，至于什么发生这件事情和如何去监听都是网络模块的事情

<总结>:
    我们可以把后端的代码分为俩个模块，一个是网络模块代码，另外一个是业务模块代码
    业务模块只关系用户的建立连接事件、断开连接事件、可读事件、可写事件，即发生这些事件后该怎么处理
    至于这些事件什么时候发生，如何去检测这些事件都是网络模块的事情
    大大节省了我们程序员的开发效率
*/



#include <mymuduo/TcpServer.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace placeholders;

/* 基于muduo网络库开发服务器程序
1. 组合TcpServer对象

2. 创建EventLoop事件循环对象的指针 (epoll)

3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
        TcpServer(EventLoop* loop, // 事件循环
            const InetAddress& listenAddr, // IP Port
            const string& nameArg, // 服务器的名字
            Option option = kNoReusePort
        );
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数

5. 设置合适的服务器线程数量，muduo库会自己分配I/O线程和worker线程

*/
class ChatServer{
public:
    /* 3 */
    ChatServer(EventLoop* loop,  // 事件循环
            const InetAddress& listenAddr, // IP + Port 
            const string& nameArg) // 服务器的名称
            :_server(loop, listenAddr, nameArg), _loop(loop)
    {
        /*
        <回调函数>：
            网络模块知道该函数什么时候发生
            业务模块知道该函数发生后怎么做
            该函数就是回调函数
            业务模块要注册回调函数，也就是告诉函数具体做什么
        */
        /* 4 */
        // 给服务器注册用户建立连接和断开连接的回调
        _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读事件和写事件的回调
        _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
        
        /* 5 */
        // 设置服务器端的线程数量 1个IO线程 3个worker线程
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void start(){
        _server.start();
    }

private:
    TcpServer _server; /* 1 */
    EventLoop *_loop; /* 2 */
    
    // 建立连接和断开连接事件的回调函数
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()) // 用户发生连接事件
        {
            cout << conn->peerAddress().toIpPort() << "->" << 
            conn->localAddress().toIpPort() << " state:online" << endl;
        }
        else // 用户发生连接断开事件
        {
            cout << conn->peerAddress().toIpPort() << "->" << 
            conn->localAddress().toIpPort() << " state:offline" << endl;
            conn->shutdown(); // close(fd)
            // _loop->quit();
        }
        
    }

    // 可读可写事件的回调函数
    void onMessage(const TcpConnectionPtr& conn, // 连接
                    Buffer *buffer, // 缓冲区
                    Timestamp time) // 接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data: " << buffer << " time：" << time.toString() << endl;
        conn->send(buf);
    }
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // lfd epoll_ctl -->epoll
    loop.loop(); // epoll_wait以阻塞的方式等待新用户的连接，已连接用户的读写事件等

    return 0;
} 