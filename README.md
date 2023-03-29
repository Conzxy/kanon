# kanon

## Introduction
`kanon`是一个使用`C++11`编写的基于**事件驱动**(event-driven)的网络库(network libaray)。<br>
具体来说，该库基于`Reactor`模式，其网络模型是`同步非阻塞`（synchronized-unblocking），依赖的的API是：`poll()`(unix-like/linux)，`epoll`(linux)、`GetQueuedCompletionStatusEx()`(Windows)。<br>
但是，实际上该库是暴露**回调注册接口**来处理各种IO事件，因此在使用上是类似`异步`的，这也是事件驱动的一个体现和优势。

除此之外，为了充分利用*多核优势*，该库支持启动多个线程，而*主线程仅接受(accept)连接*，而这些线程处理*IO事件*，因此一般这些线程称作`IO线程`。

> 旧：该库目前不考虑跨平台，因为`Windows`的网络API不贴近`Reactor`，兼容的话要承担一定的额外开销，同时，对现在的我而言也没这个必要。
> 新：现在kanon支持Windows的**Client** ，底层采用IOCP抽象为Poller来进行同步等待。

另外，该库也实现了其他有用的组件，它们也是构成网络库的一部分，但对于编写应用程序的其他*非网络模块*也是十分有用的:
| 模块 | 描述 | 相关文件 |
| -- | -- | -- |
| log | 支持输出到`终端`（terminal）/`文件`(*同步*或**异步**）| /kanon/log |
| thread | Pthread：`互斥锁`（mutex），`条件变量`（condition），以及基于此实现的`CountdownLatch`等 | /kanon/thread |
| string | `string_view`的11等价实现，字符流和格式化流等 | /kanon/string |
| util | `std::optional`，`make_unique()`的11等价实现，`noncopyable`等 | /kanon/util |
| algo | 支持O(1) append的单链表，支持`reallocate`的预分配数组等 | /kanon/algo |
| ... | ... | ... |

更多的可以通过源码了解。

## Buffer
对于网络库而言，想必对其使用的**读写缓冲区**(read/write buffer)很感兴趣，因为涉及**收发信息**(receive/send message)的性能。

| 缓冲 | 描述 | 相关文件 |
| -- | -- | -- |
| kanon::Buffer | **读**缓冲，支持*prepend size header*的**连续**容器，由于是基于`kanon::ReservedArray`实现的，因此比基于`std::vector`的性能要好 | algo/reserved_array.h, buffer/buffer.h,cc | 
| kanon::ChunkList | **写**缓冲，**固定页面**的单链表（支持O(1) append)，分配的节点除非用户主动收缩，否则不释放，自然也支持*prepend size header*（因为每个节点本身就支持可变长度），细节参考相关文件 | algo/forward_list.h, algo/forward_list/\*, buffer/chunk_list.h,cc |

之所以这么设计，是因为接受的信息一般需要**解析**/**反序列化**(parse/deserialize)，因此如果不是连续的，那么得付出拼接完整信息的额外开销（overhead），这是划不来的，所以采用特化的连续容器。<br>
而对于发送消息，我们并不关心其完整性，因此采用*基于节点*的非连续容器，即单链表可以避免因连续容器再分配带来的*memcpy*的开销。
实际上，通过`ReservedArray`实现的`Buffer`在一些情况下是可以进行*原地再分配*的(inplace reallocate)，因此*benchmark*的结果表现略优于`ChunkList`，但根据现实的*工作负载*(workload)来考虑，写缓冲还是考虑用`ChunkList`，在我看来，这至少不是个坏主意(Bad IDEA)。

写缓冲支持size header的O(1) prepend，在我看来是个很不错的想法，在处理二进制协议时，这是十分有必要的，因此`Buffer`和`ChunkList`都支持这个特性。

## Build
`kanon`是通过`cmake`构筑的。因此你应该先安装`cmake`。

如果你使用的是`Ubuntu` OS，那么可以通过以下命令安装：
```shell
$ sudo apt install cmake
```

然后通过以下命令构筑该库：
```shell
$ git clone https://github.com/conzxy/kanon
$ cd kanon
$ mkdir build && cd build
# Setting KANON_BUILD_STATIC_LIBS can choose to generate shared libaray(.so) or static libaray(.a)(default:ON)
# More options please see:
# cmake .. -LH
$ cmake ..
$ cmake --build . --parallel $(nproc)
```

构筑完成后，你可以安装该库：
```shell
# In */kanon/build
$ cmake --install .
```

如果你要修改安装路径，可以按照如下命令：
```shell
# 路径可以是绝对路径也可以是相对路径
# 如果是相对路径，CMake会认为这是相对prefix的路径
# prefix指定命令：
# cmake --install . --prefix ...
$ cmake .. -DCMAKE_INSTALL_INCLUDEDIR=... -DCMAKE_INSTALL_LIBDIR=...
```

## Example
一个简单的例子是[daytime](https://www.ietf.org/rfc/rfc867.txt)服务器。<br>
根据`daytime`协议，我们仅需要注册`OnConnection`回调即可。
```cpp
void OnConnection(TcpConnectionPtr const& conn)
{
  conn->SetWriteCompleteCallback([](TcpConnectionPtr const& conn) {
    conn->ShutdownWrite();
  });

  conn->Send(GetDaytime());
}
```
除此之外，你也可以注册`OnMessage`回调以处理信息（比如请求等），还有其他回调你也可以参考[tcp_connection.h](https://github.com/Conzxy/kanon/blob/master/kanon/net/tcp_connection.h)。

其他的例子可以参考[example](https://github.com/Conzxy/kanon/tree/master/example) 目录。
## Document
目前仅有网络模块的API文档，是通过`Doxygen`生成的。

> 目前已关闭
Website: http://47.99.92.230/
（由[kanon_httpd](https://github.com/Conzxy/kanon_httpd)支持）

