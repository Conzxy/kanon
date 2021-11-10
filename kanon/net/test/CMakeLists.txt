# add_executable(EventLoop_test EventLoop_test.cc ${KANON_SRC})
# add_executable(EventLoop_test2 EventLoop_test2.cc ${KANON_SRC})
#add_executable(EventLoop_test3 EventLoop_test3.cc ${KANON_SRC})
#add_executable(timer_test1 timer_test1.cc ${KANON_SRC})
#add_executable(Acceptor_test Acceptor_test.cc ${KANON_SRC})
#add_executable(timer_test2 timer_test2.cc ${KANON_SRC})
#add_executable(TcpServer_test TcpServer_test.cc ${KANON_SRC})

add_executable(EventLoopThread_test EventLoopThread_test.cc ${KANON_SRC})

add_executable(SIGPIPE_test SIGPIPE_test.cc ${KANON_SRC})
add_executable(InetAddr_test InetAddr_test.cc ${KANON_SRC})
target_link_libraries(InetAddr_test gtest)

add_executable(Buffer_test Buffer_test.cc ${KANON_SRC})
target_link_libraries(Buffer_test gtest)

add_executable(Connector_test Connector_test.cc ${KANON_SRC})

add_executable(TcpClient_test TcpClient_test.cc ${KANON_SRC})