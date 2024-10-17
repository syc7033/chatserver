#pragma once

#include "user.hpp"

// 该类继承与User类 多了一个用户权限属性
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

private:
    string role;
};