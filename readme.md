# Chatter 聊天客户端
[English Version](#Chatter-Chat-Server)  
这是一个基于 Qt 6.5.3 和 C++17 开发的实时聊天客户端，用于配合服务端项目 [chatter server](https://github.com/Garhlz/chatter2_server)。界面友好、功能完整，适合在本地环境下测试和部署使用。
# 功能
- [x] 用户认证
    - 登录/注册
    - JWT 令牌验证
    - 会话管理
- [x] 聊天大厅
    - 公共聊天大厅（消息广播）
    - 历史消息记录
    - 服务器消息同步
- [x] 私聊功能
    - 用户在线状态
    - 私聊会话管理
    - 历史消息记录
    - 服务器消息
- [x] 群聊功能
    - 群组创建与解散
    - 群成员管理
- [x] 文件传输
    - 通过 Spring Boot HTTP 实现上传 / 下载
    - 支持传输进度展示
    - 历史文件访问

# Chatter Chat Server
[中文版](#Chatter-聊天客户端)  
A real-time chat client built with Qt 6.5.3 and C++17, designed to work seamlessly with the [chatter server](https://github.com/Garhlz/chatter2_server). It offers a complete and friendly interface for local usage, testing, or further extension.

# Features
- [x] User Authentication

    - Login / Registration

    - JWT token verification

    - Session management

- [x] Public Chatroom

    - Broadcast messages

    - Message history

    - Server-side synchronization

- [x] Private Messaging

    - Online user status

    - Private session management

    - Message history

- [x] Group Chat

    - Group creation and deletion

    - Member management

- [x] Message Handling

    - Persistent message storage

    - Error handling

- [x] File Transfer

    - File upload/download via Spring Boot HTTP

    - Transfer status and progress display

    - Download historical files
