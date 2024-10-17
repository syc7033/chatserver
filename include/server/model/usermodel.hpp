#pragma once

#include "user.hpp"

// User表的数据操作类
class UserModel
{
public:
    // User表的增加方法
    bool insert(User &user);

    // 根据用户号码查询用户信息
    User query(int id);

    // 修改用户的登录信息
    bool updateState(User &user);

    // 服务器异常结束 重置用户的状态信息
    void resetState();
private:

};