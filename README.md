# TTServer

TTServer主要包含了以下几种服务器:

- **LoginServer**: 负载均衡服务器，分配一个负载小的MsgServer给客户端使用
- **MsgServer**: 消息服务器，提供客户端大部分信令处理功能，包括私人聊天、群组聊天等
- **RouteServer**: 路由服务器，为登录在不同MsgServer的用户提供消息转发功能
- **FileServer**: 文件服务器，提供客户端之间得文件传输服务，支持在线以及离线文件传输
- **MsfsServer**: 图片存储服务器，提供头像，图片传输中的图片存储服务
- **DBProxy**: 数据库代理服务器，提供mysql以及redis的访问服务，屏蔽其他服务器与mysql与redis的直接交互
- **HttpMsgServer**: 对外接口服务器，提供对外接口功能。（目前只是框架）
- **PushServer**: 消息推送服务器，提供IOS系统消息推送。（IOS消息推送必须走apns）

## 服务端流程

服务端的启动没有严格的先后流程，因为各端在启动后会去主动连接其所依赖的服务端，如果相应的服务端还未启动，会始终尝试连接。

不过在此，如果是线上环境，还是建议按照如下的启动顺序去启动(也不是唯一的顺序)：

1. 启动`db_proxy`。

2. 启动`route_server`，`file_server`，`msfs`

3. 启动`login_server`

4. 启动`msg_server`

那么我就按照服务端的启动顺序去讲解服务端的一个流程概述。

1. 启动`db_proxy`后，`db_proxy`会去根据配置文件连接相应的`MySQL`实例，以及`redis`实例。

2. 启动`route_server`，`file_server`，`msfs`后，各个服务端都会开始监听相应的端口。

3. 启动`login_server`，`login_server`就开始监听相应的端口(8080)，等待客户端的连接，而分配一个负载相对较小的`msg_server`给客户端。

4. 启动`msg_server`(端口8000)，`msg_server`启动后，会去主动连接`route_server`，`login_server`，`db_proxy_server`，会将自己的监听的端口信息注册到`login_server`去，同时在用户上线，下线的时候会将自己的负载情况汇报给`login_server`。

## 服务端口号

| 服务              | 端口        |
| ----------------- | ----------- |
| login_server      | 8080/8008   |
| msg_server        | 8000        |
| db_proxy_server   | 10600       |
| route_server      | 8200        |
| http_msg_server   | 8400        |
| file_server       | 8600/8601   |
