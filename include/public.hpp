#pragma once

/**
 * client 和 server的公共文件
 */
// {"msgid":1,"id":23,"password":"123456"}
enum EnMsgType
{
    LOGIN_MSG = 1, // 登录消息
    LOGIN_MSG_ACK, // 登录消息响应
    LOGINOUT_MSG, // 注销消息
    REG_MSG,  // 注册消息
    REG_MSG_ACK, // 注册消息响应
    ONE_CHAT_MSG, // 聊天消息(1对1)
    ADD_FRIEND_MSG, // 添加好友消息

    CREATE_GROUP_MSG, // 创建群组消息
    ADD_GROUP_MSG, // 加入群组消息
    GROUP_CHAT_MSG, // 群聊消息

};
