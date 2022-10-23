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

2022-9-1 Conzxy
 * 实现protobuf的`ZeroCopyOutputStream`接口：`ChunkStream`，实现零拷贝以最大化的`ChunkList`性能
 * Refactor: `Protobuf`和`ProtobufRpc`模块

2022-9-3 Conzxy
 * protobuf提供`Callable`工具类包装任意可调用对象（取代预定义的劣包装）
 * 采用xxHash作为校验和的哈希算法
   * 性能优异
   * 支持流式数据，以适合ChunkList

2022-9-8 Conzxy
 * Fix: 调用`quit()`多次触发
 * thread package: `ThreadLocal` utility
 * 修改`InetAddr`部分函数以符合通用格式(host:port)
 * 实现`RpcController`支持请求超时取消

2022-9-14 Conzxy
 * 全局日志开关：`EnableAllLog()`，控制kanon的所有日志输出行为

2022-9-15 Conzxy
 * Fix: `IsSelfConnect()`, 如果正在连接，获取不到地址是正常的，不应视为自连接错误

2022-9-21 Conzxy
 * Update: Server的信号处理函数都直接退出该进程，资源由OS回收

2022-10-01 Conzxy
 * Add: 解决`Any`不支持move-only对象的问题（当然，可以在外面动态分配传进来，但是需要两次动态分配开销），实现`UniqueAny`仅支持移动（语义同`std::unique_ptr`）
 * Add: 由于移动会导致回调绑定的this失效，因此对象还是动态分配比较好，但是由于不想涉及两次动态分配，还是编写了`RawAny`（一个`void*` wrapper），只不过生命周期需要用户自己管理（就目前来说，还是足够易用的）。（尽管可以让对象实现一个方法，让新对象重新绑定回调，但略为不自然，这里不考虑这种做法）。
 * Update: 将`TcpConnection`的上下文类型换成了`RawAny`。

2022-10-04 Conzxy
 * Add: 添加`MakeSharedFromProtected()`工具函数，令TcpConnection，TcpClient，Connector的构造函数为protected，避免用户误用，强制使用`NewXXX()`函数进行正确的对象创建。
 * 调整Connector，TcpClient部分代码（和添加注释）

2022-10-20 Conzxy
 * Add: 添加`FixedChunkMemoryPool`，将对象的内存缓存到固定分块的内存池中，从而可以尽量减少调用`malloc()`的次数和减少内存碎片（主要是外部碎片）出现的概率，
   避免服务器程序长时间运行内存降不下来（e.g. TcpConnection，\*Session（用户自定义））
 * Add: TcpServer支持设置连接内存池（自然，也可以采用通常的`malloc()/free()`）
