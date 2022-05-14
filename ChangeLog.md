2022-4-17 Conzxy 
 * 优化WriteCompleteCallback的流水线写入，不会在中间的块写完之后立马不关注写事件（具体参考`tcp_connection.cc`的注释）

2022-4-29 Conzxy
 * 日志模块加入了一些其他选项，可以指定日志堆积到一定程度自动删除

2022-5-13 Conzxy
 * `RpcChannel::CallMethod()`使用`EventLoop::RunInLoop()`避免使用锁保证call_result线程安全
 * Rpc只管理`RpcServer`填充的Response生命周期（`SendRpcResponse()`中释放），其他的均由用户管理

2022-5-14 Conzxy
 * 根据环境变量的值而不是有无来初始化日志级别，这样没有必要重启shell(因为`source xx/bash_profile`没用)
 * 提供Log宏的wrapper，对于kanon库的*TRACE*和*DEBUG*日志信息可以通过变量控制（default: false(Release), true(Debug))
 * 构造TcpServer的同时注册`SIGINT`，`SIGTERM`，`SIGKILL`的处理器：退出事件循环