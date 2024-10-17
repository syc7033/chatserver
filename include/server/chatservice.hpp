#pragma once
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

#include <mymuduo/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace std;
using json = nlohmann::json;

// 处理事件的回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp time)>;


/* 聊天服务器业务类 */ 
class ChatService : noncopyable // 相当于删除了拷贝构造和赋值运算符的重载
{
public:
    // 单例模式的全局访问结点
    static ChatService& getInstance();

    // 通过消息类型获取响相应的处理器
    MsgHandler getHandler(int msgid);

    // 处理客户端异常退出
    void clinetCloseException(const TcpConnectionPtr& conn);

    // 服务器异常结束 业务重置方法
    void reset();

    // 处理登录业务
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr&conn, json& js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr&conn, json& js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr&conn, json& js, Timestamp time);

    // 用户加入群组业务
    void addGroup(const TcpConnectionPtr&conn, json& js, Timestamp time);

    // 群聊业务
    void groupChat(const TcpConnectionPtr&conn, json& js, Timestamp time);

    // 处理注销业务
    void loginout(const TcpConnectionPtr&conn, json& js, Timestamp time);

    // 跨服务器通信 上报消息的回调
    void handleRedisSubscribeMessage(int, string);
private:
    ChatService();

    // 消息类型对应的业务处理方法 <mgsid, function<函数类型>>
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 数据层 操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // 存储在线用户的通信连接 <id, TcpConnection>
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁 保证_userConnMap的线程安全
    mutex _connMutex;

    // redis操作对象
    Redis _redis;
};