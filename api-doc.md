# 聊天服务器接口文档

## 概述
本文档描述了聊天服务器的 Socket 接口规范。服务器使用 JSON 格式进行数据交互，支持 TCP 长连接。

## 服务器信息
- Socket 服务端口：9999
- 心跳间隔：30秒（可在application.yml中配置）
- 心跳超时：90秒（可在application.yml中配置）
- 最大消息长度：10000字节

## 安全认证
- 除登录和注册外，所有请求都需要携带JWT token
- token在登录成功后获取
- token有效期为24小时
- token通过响应消息的token字段返回

## 基础消息格式
所有消息使用JSON格式，基本结构如下：

```json
{
  "type": "消息类型",
  "username": "用户名",
  "password": "密码",
  "nickname": "昵称",
  "receiver": "接收者",
  "groupName": "群组名称",
  "content": "消息内容",
  "messageId": "消息ID",
  "token": "JWT令牌",
  "status": "状态",
  "timestamp": "时间戳",
  "errorMessage": "错误信息",
  "onlineUsers": "在线用户列表",
  "onlineCount": "在线用户数量"
}
```

## 消息类型（MessageType）
- LOGIN: 登录请求
- LOGOUT: 登出请求
- REGISTER: 注册请求
- CHAT: 普通聊天消息
- PRIVATE_CHAT: 私聊消息
- GROUP_CHAT: 群聊消息
- FILE: 文件传输
- HEARTBEAT: 心跳包
- SYSTEM: 系统消息
- ERROR: 错误消息

## 详细接口说明

### 1. 用户认证

#### 1.1 用户注册
- 请求类型: REGISTER
- 必填字段: username, password, nickname
- 响应: 成功返回用户信息和token，失败返回错误信息

```json
// 请求示例
{
  "type": "REGISTER",
  "username": "user123",
  "password": "password123",
  "nickname": "张三"
}

// 成功响应
{
  "type": "SYSTEM",
  "content": "注册成功",
  "token": "eyJhbGciOiJIUzI1NiJ9...",
  "status": "success",
  "timestamp": "2023-05-01T12:00:00"
}

// 失败响应
{
  "type": "ERROR",
  "errorMessage": "用户名已存在",
  "status": "error",
  "timestamp": "2023-05-01T12:00:00"
}
```
{"type":"REGISTER","username":"222","password":"222","nickname":"User Two"}
{"type":"LOGIN","username":"111","password":"111"}
#### 1.2 用户登录
- 请求类型: LOGIN
- 必填字段: username, password
- 响应: 成功返回用户信息和token，失败返回错误信息

```json
// 请求示例
{
  "type": "LOGIN",
  "username": "user123",
  "password": "password123"
}

// 成功响应
{
  "type": "SYSTEM",
  "content": "登录成功",
  "token": "eyJhbGciOiJIUzI1NiJ9...",
  "status": "success",
  "timestamp": "2023-05-01T12:00:00",
  "onlineUsers": [{"username":"user123","nickname":"张三","status":"online"}],
  "onlineCount": 1
}

// 失败响应
{
  "type": "ERROR",
  "errorMessage": "用户名或密码错误",
  "status": "error",
  "timestamp": "2023-05-01T12:00:00"
}
```

#### 1.3 用户登出
- 请求类型: LOGOUT
- 必填字段: token
- 响应: 成功返回确认信息

```json
// 请求示例
{
  "type": "LOGOUT",
  "token": "eyJhbGciOiJIUzI1NiJ9..."
}

// 成功响应
{
  "type": "SYSTEM",
  "content": "已成功登出",
  "status": "success",
  "timestamp": "2023-05-01T12:00:00"
}
```

### 2. 消息通信

#### 2.1 发送公共聊天消息
- 请求类型: CHAT
- 必填字段: content, token
- 响应: 消息广播给所有在线用户

```json
// 请求示例
{
  "type": "CHAT",
  "content": "大家好！",
  "token": "eyJhbGciOiJIUzI1NiJ9..."
}

// 广播消息
{
  "type": "CHAT",
  "username": "user123",
  "nickname": "张三",
  "content": "大家好！",
  "timestamp": "2023-05-01T12:00:00",
  "messageId": 12345
}
```

#### 2.2 发送私聊消息
- 请求类型: PRIVATE_CHAT
- 必填字段: receiver, content, token
- 响应: 消息发送给指定用户

```json
// 请求示例
{
  "type": "PRIVATE_CHAT",
  "receiver": "user456",
  "content": "你好，这是私聊消息",
  "token": "eyJhbGciOiJIUzI1NiJ9..."
}

// 接收方收到的消息
{
  "type": "PRIVATE_CHAT",
  "username": "user123",
  "nickname": "张三",
  "content": "你好，这是私聊消息",
  "timestamp": "2023-05-01T12:00:00",
  "messageId": 12346
}
```

#### 2.3 发送群聊消息
- 请求类型: GROUP_CHAT
- 必填字段: groupName, content, token
- 响应: 消息发送给群组内所有成员

```json
// 请求示例
{
  "type": "GROUP_CHAT",
  "groupName": "技术交流群",
  "content": "大家看看这个问题怎么解决？",
  "token": "eyJhbGciOiJIUzI1NiJ9..."
}

// 群成员收到的消息
{
  "type": "GROUP_CHAT",
  "username": "user123",
  "nickname": "张三",
  "groupName": "技术交流群",
  "content": "大家看看这个问题怎么解决？",
  "timestamp": "2023-05-01T12:00:00",
  "messageId": 12347
}
```

#### 2.4 文件传输
- 请求类型: FILE
- 必填字段: receiver/groupName, content(Base64编码的文件内容), token
- 响应: 文件发送给指定用户或群组

```json
// 请求示例 (私聊文件)
{
  "type": "FILE",
  "receiver": "user456",
  "content": "base64编码的文件内容...",
  "token": "eyJhbGciOiJIUzI1NiJ9..."
}

// 接收方收到的消息
{
  "type": "FILE",
  "username": "user123",
  "nickname": "张三",
  "content": "base64编码的文件内容...",
  "timestamp": "2023-05-01T12:00:00",
  "messageId": 12348
}
```

### 3. 系统功能

#### 3.1 心跳包
- 请求类型: HEARTBEAT
- 必填字段: token
- 说明: 客户端需要定期发送心跳包以维持连接

```json
// 请求示例
{
  "type": "HEARTBEAT",
  "token": "eyJhbGciOiJIUzI1NiJ9..."
}

// 响应示例
{
  "type": "HEARTBEAT",
  "timestamp": "2023-05-01T12:00:00"
}
```

#### 3.2 系统通知
- 类型: SYSTEM
- 说明: 服务器主动推送的系统消息，如用户上线/下线通知等

```json
// 系统通知示例
{
  "type": "SYSTEM",
  "content": "用户 '李四' 已上线",
  "timestamp": "2023-05-01T12:00:00",
  "onlineUsers": [{"username":"user123","nickname":"张三","status":"online"}, {"username":"user456","nickname":"李四","status":"online"}],
  "onlineCount": 2
}
```

#### 3.3 错误消息
- 类型: ERROR
- 说明: 服务器返回的错误信息

```json
// 错误消息示例
{
  "type": "ERROR",
  "errorMessage": "消息发送失败：接收用户不存在",
  "status": "error",
  "timestamp": "2023-05-01T12:00:00"
}
```

## 错误码说明

| 错误码 | 描述 |
| ----- | ---- |
| 1001  | 认证失败 |
| 1002  | 用户不存在 |
| 1003  | 密码错误 |
| 1004  | 用户名已存在 |
| 2001  | 消息格式错误 |
| 2002  | 消息发送失败 |
| 3001  | 群组不存在 |
| 3002  | 不是群组成员 |
| 9999  | 服务器内部错误 |
