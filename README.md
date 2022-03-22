# Kanon
```
 _  __                       
| |/ /                       
| ' / __ _ _ __   ___  _ __  
|  < / _` | '_ \ / _ \| '_ \ 
| . \ (_| | | | | (_) | | | |
|_|\_\__,_|_| |_|\___/|_| |_|
```
## Introduction
`Kanon` is an event-driven network library written in `C++11`(Support `TCP` only). The library must be be used in `linux` or `unix-like` platform(since depends on the API of them).
Therefore, the event-handling is synchronized-unblocking(`poll()` or `epoll()`). Due to this feature, it is natural to implement the network module of library in the `reactor` design pattern. Also in order to take the advantage of the mutil-core machine, the library can start many thread, and the main thread accept connection, other threads(**IO threads**) handle IO events, that including receiving messages, processing messages, sending messages, etc. This is called as `multi-reactor`.

In addition, the library also implements other useful components:
* **log module**(terminal/file/async logging)
* **thread module**(Pthread: mutex, condition variable, etc.)
* **string module**(lexical_stream, string_view, etc.)
* **util module**(down_pointer_cast, make_unique(c++11), etc.)

These modules can be reuse in other places also.

## Build
The library is built by `cmake`.

The cmake minimum version I set is **3.10**, but I don't know which command is also allowed under 3.10. But it is best that you ensure the cmake version is 3.10 at least.

If you use `Ubuntu` OS, you can install cmake by following command:
```shell
$ sudo apt install cmake
```
Then, build `kanon` by following commands:
```shell
# ${USER_ROOT_DIR} is ~ usually.
$ cd ${USER_ROOT_DIR}/kanon
$ mkdir build && cd build
# Setting BUILD_SHARED_LIB can choose to generate shared libaray(.so) or static libaray(.a)
# Setting BUILD_ALL_TESTS can choose whether to generate tests or not
# Setting BUILD_ALL_EXAMPLES can choose whether to generate example or not
$ cmake .. -D BUILD_SHARED_LIBS=ON -D BUILD_ALL_TESTS=ON -D BUILD_ALL_EXAMPLES=ON
# -j specify the number of concurrent jobs
# You can set it according to the core number of your machine
$ cmake --build . --target all -j 2
```

* The default generator is `Unix Makefiles`, you can change it to other also.For example, `Ninja` which I'm using.
The command line argument as following:
```shell
cmake -G Ninja ...
```

* The default build type is **Release**, if you want to build **Debug** mode, you can input following command:
```shell
$ cmake .. -D CMAKE_BUILD_TYPE=Debug -D BUILD_SHARED_LIBS=ON -D BUILD_ALL_TESTS=ON
```

* The default output directory of library is `${USER_ROOT_DIR}/kanon/build/lib`.
* The output directory of tests is `${USER_ROOT_DIR}/kanon/build/test`
* The output directory of examples is `${USER_ROOT_DIR}/kanon/build/examples`

After build, you can install headers to `/usr/include/`, and install libraries to `/usr/lib` default.
```shell
# In ${USER_ROOT_DIR}/kanon/build
$ cmake --install .
```
You can change the install directory of libraries to other by the following command:
```shell
# ${INSTALL_DIR_YOU_WANT} is the install directory of lib you want
$ cmake --install . --prefix ${INSTALL_DIR_YOU_WANT}
```
## Example
The simple example is [daytime](https://www.ietf.org/rfc/rfc867.txt) server.
According the daytime protocol, we just register the **OnConnection** callback.
```cpp
void OnConnection(TcpConnectionPtr const& conn)
{
  conn->SetWriteCompleteCallback([](TcpConnectionPtr const& conn) {
    conn->ShutdownWrite();
  });

  conn->Send(GetDaytime());
}
```
Besides, you also can register OnMessage callback to process message from peer, etc. You can know them in [tcp_connection.h](https://github.com/Conzxy/kanon/blob/master/kanon/net/tcp_connection.h).
Other examples in [example](https://github.com/Conzxy/kanon/tree/master/example) directory.