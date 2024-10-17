#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <functional>
#include <string>

using namespace std;
using namespace placeholders;

using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
            const InetAddress &listenAddr,
            const std::string &nameArg)
    : server_(loop, listenAddr, nameArg)
    , loop_(loop)
{
    // 注册用户连接的回调
    server_.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    // 注册有读写事件发生的回调
    server_.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置开启subLoop的数量
    server_.setThreadNum(4);
}

void ChatServer::start()
{
    // 创建子线程 子事件循环并开启
    server_.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    // 客户端断开连接
    if(!conn->connected())
    {
        ChatService::getInstance().clinetCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time)
{
    /**
     * decoing(json字符串) => json对象 => 取出msgid
     * 调用业务模块的Map找到该消息类型对应的Handler 该操作也可认为是网络层与业务层的解耦操作
     * 解耦是为了使模块与模块之间相对独立，一个模块不会应为另外已给模块的改变都造成太大的影响
     */
    string buf = buffer->retrieveAllAsString();
    cout << "ChatServer recv json " << buf << endl;
    // 数据的反序列化 decoding
    json js = json::parse(buf);
    // 目的: 完全解耦网络模块和业务模块代码
    // js["msgid"] 获取 => 业务handler => conn js time
    auto handler = ChatService::getInstance().getHandler(js["msgid"].get<int>());
    // 回调消息绑定号的事件处理器 起始就一个Function函数对象
    handler(conn, js, time);
}
