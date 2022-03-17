# Kanon
## Introduction
`Kanon` is an event-driven network library(Support TCP only). The library must be be used in linux or unix-like platform(since depends on the API of them).
Therefore, the event-handling is synchronized-unblocking(`poll()` or `epoll()`). Due to this feature, it is natural to implement the network module of library in the `reactor` design pattern. Also in order to take the advantage of the mutil-core machine, the library can start many thread, and the main thread accept connection, other threads(**IO threads**) handle IO events, that including receiving messages, processing messages, sending messages, etc. This is called as `multi-reactor`.

In addition, the library also implements other useful component:
* log module(terminal/file/async logging)
* thread module(Pthread: mutex, condition variable, etc.)
* string module(lexical_stream, string_view, etc.)
* util module(down_pointer_cast, make_unique(c++11), etc.)

These modules can be reuse in other place also.

## Build
The library is built by cmake.

The cmake minimum version is 3.10, but I don't know which command is also allowed under 3.10. But it is best that you ensure the cmake version is 3.10 at least.

If you use Ubuntu OS, you can install cmake by following commands
```shell
$ sudo apt install cmake
```
Then, build kanon by following commands
```shell
# ${USER_ROOT_DIR} is ~ usually.
$ cd ${USER_ROOT_DIR}/kanon
$ mkdir build && cd build
# Setting BUILD_SHARED_LIB can choose to generate shared libaray(.so) or static libaray(.a)
# Setting BUILD_ALL_TESTS can choose whether to generate tests or not
$ cmake .. -D BUILD_SHARED_LIBS=ON -D BUILD_ALL_TESTS=ON
$ cmake --build .
```
The default output directory of library is `${USER_ROOT_DIR}/kanon/build/lib`.
The output directory of tests is `${USER_ROOT_DIR}/kanon/build/test`

After build, you can install headers to `/usr/include/`, and install libraries to `/usr/lib` default.
```shell
# In ${USER_ROOT_DIR}/kanon/build
$ cmake --install .
```
You can change the install directory of libraries to other by the following command:
```shell
# ${YOU_WANT_INSTALL_DIR} is the install directory of lib you want
$ cmake --install . -prefix ${YOU_WANT_INSTALL_DIR}
```