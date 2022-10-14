# kanon echo server
add_executable(kanon-echo-server kanon_echo_server.cc)
target_link_libraries(kanon-echo-server kanon_base kanon_net)

# qt echo server
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(qt-echo-server qt_echo_server.cc)
set(QT_VERSION Qt5)
find_package(${QT_VERSION} REQUIRED COMPONENTS Core Network)
target_link_libraries(qt-echo-server ${QT_VERSION}::Core ${QT_VERSION}::Network)

# libevent echo server
add_executable(libevent-echo-server libevent_echo_server.c)
target_link_libraries(libevent-echo-server event)
