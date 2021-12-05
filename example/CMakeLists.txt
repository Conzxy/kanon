add_executable(discard discard.cc ${KANON_SRC})
add_executable(connects connects.cc ${KANON_SRC})
add_executable(EchoServer EchoServer.cc ${KANON_SRC})

add_subdirectory(chat)
add_subdirectory(http)
add_subdirectory(memcache)
