#pragma once

#include <string>

using namespace std;

/* 匹配User表的ORM类对象 映射数据库中表的字段 */
class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state;
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPwd(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPwd() { return this->password; }
    string getState() { return this->state; }
protected: 
    int id; // 用户id
    string name; // 用户昵称
    string password; // 用户密码
    string state; // 用户的登录状态
};