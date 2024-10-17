#include "chatservice.hpp"
#include "public.hpp"

#include <functional>
#include <mymuduo/Logger.h>
#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace placeholders;

/**     业务层与数据层的解耦
 * 通过ORM框架结构来实现的, 具体如下
 * 对于数据库中的每一个表, 我们都创建已给类与他建立映射关系, 表中字段对应类中的属性
 * 对于操作数据库的sql增删改查 我们创建一个model类 去建立映射
 * 这样做的好处是 我们业务层看见的都是数据对象 不用直接写sql语句
 * 换句话来说数据层怎么变都我的业务层不产生影响
 * 比如公司今天想用mysql 明天 想用dgdb 那么修改数据层就可以 达到了解耦的效果
 */

ChatService& ChatService::getInstance()
{
    static ChatService service;
    return service;
}

// 注册消息以及对应的回调操作 (msgHanlerMap)

/**业务层的构造函数要干什么?
 * 通过hashMap给不同的消息类型绑定对应的Handler 回调
 */
ChatService::ChatService()
{
    // 用户基本业务管理相关事件的回调操作
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件回调函数注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3)});    

    // 连接Redis服务器
    if(_redis.connect())
    {
        // 上报消息的回调
        _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 服务器异常结束 业务重置方法
void ChatService::reset()
{
    _userModel.resetState();
}

// 通过消息类型获取相应处理函数的接口 => 网络层调用
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志, msgid 没有相应事件的回调
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        //如果msgid没有实现相应的回调 我们需要返回一个空msgHandler 防止我们的网络模块出现段错误 造成网络模块崩溃
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp time)
        {
            LOG_ERROR("msgid = %d can not find handler!", msgid);
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}




// 处理登录业务

/** 接口细节
 * 1.获取id对应的pwd(数据层), 并验证
 * 2.修改用户的登录状态(数据层) offline => online
 * 3.判断用户是否有离线消息(数据层) 获取完 删除离线消息
 * 4.拉去用户的好友信息
 */
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);

    json response;
    response["msgid"] = LOGIN_MSG_ACK;

    if(user.getId() == id && user.getPwd() == pwd)
    {
        if(user.getState() == "online")
        {
            // 该用户已经登录不允许重复登录
            response["errno"] = 2;
            response["errmsg"] = "The account has already been logged in.";
        }
        else
        {
            // 登录成功 服务器记录连接信息
            {
                /* 要注意锁的粒度控制 不要从头到尾都加锁 那和串行没啥区别了 */
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({user.getId(), conn});
            }

            // id用户登录成功后 向reids订阅channel
            _redis.subscribe(id);

            // 登录成功 更新用户的状态信息
            user.setState("online");
            _userModel.updateState(user);

            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> offlineMsgs = _offlineMsgModel.query(id);
            if(!offlineMsgs.empty())
            {
                // 有离线小 读取 放到json对象中
                response["offlinemsg"] = offlineMsgs;
                // 读完就删掉
                _offlineMsgModel.remove(id);
            }

            //  查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty())
            {
                vector<string> vec;
                for(User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec.push_back(js.dump());
                }
                response["friends"] = vec;
            }

             // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getGpUserVec())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }
        }
        
    }
    else
    {
        // 登录失败 要么用户名不存在 要么用户名或者密码错误
        response["errno"] = 1;
        response["errmsg"] = "Username or password incorrect";
    }
    conn->send(response.dump()); // 把json对象序列化位json字符串
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);

    json response;
    response["msgid"] = REG_MSG_ACK;

    if(state)
    {
        // 注册成功
        response["errno"] = 0;
        response["id"] = user.getId();
    }
    else
    {
        // 注册失败
        response["errno"] = 1;
        response["errmsg"] = "注册失败";
    }
    conn->send(response.dump()); // 把json对象序列化位json字符串
}

// 处理客户端异常退出
void ChatService::clinetCloseException(const TcpConnectionPtr& conn)
{
    User user;

    /*  线程安全处理(connMap)    */
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it : _userConnMap)
        {
            if(it.second == conn)
            {
                // 通过conn 找到 id
                user.setId(it.first);
                // 用map表删除用户的连接信息
                _userConnMap.erase(it.first);
                break;
            }
        }
    }// {"msgid":1,"id":22,"password":"123456"}

    // 客户端异常结束 也要从redis中取消订阅

    _redis.unsubscribe(user.getId());

    if(user.getId() != -1)
    {
        // 遍历Map表的时候找到了用户的连接
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr&conn, json& js, Timestamp time)
{
    cout << "ChatService::oneChat json: " << js.dump() << endl;
    int toId = js["toid"].get<int>();

    /* 表示用户是否在线 因为要访问connMap表 要考虑线程安全 */
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toId);
        if(it != _userConnMap.end())
        {
            // 说明 聊天的两个人在同一个服务器下登录 且都在线直接当前服务器转发就OK
            it->second->send(js.dump());
            return;
        }   
    }

    // 不在同一个服务器 查询数据库看看对方是否在线
    User user = _userModel.query(toId);
    if(user.getState() == "online")
    {
        // 对方在线 只是不在同一个服务器 把该消息发布到redis中
        _redis.publish(toId, js.dump());
        return;
    }

    // toId不在线 存储离线消息
    _offlineMsgModel.insert(toId, js.dump());

}

// 添加好友业务
/**
 * msgid id friendId
 */
void ChatService::addFriend(const TcpConnectionPtr&conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    
    // 存储好友关系
    _friendModel.insert(userid, friendid);

}


// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr&conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 用户加入群组业务
void ChatService::addGroup(const TcpConnectionPtr&conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群聊业务
void ChatService::groupChat(const TcpConnectionPtr&conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(auto id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end())
        {
            // 发送消息的人处于同一个服务器 且用户在线
            it->second->send(js.dump());
        }
        else
        {
            // 发送消息的人与接收消息的人不同一个服务器
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr&conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {   
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(userid);
        }
    }

    // 下线要取消Redis消息队列中的订阅
    _redis.unsubscribe(userid);

    // 下线更新用户状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 跨服务器通信 上报消息的回调
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            // 用户在线 直接转发
            it->second->send(msg);
            return;
        }

        // 用户不在线 存储离线消息
        _offlineMsgModel.insert(userid, msg);
    }
}