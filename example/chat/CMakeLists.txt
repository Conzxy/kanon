add_executable(ChatServer_main ChatServer_main.cc ChatServer.cc ../codec.cc ${KANON_SRC})
add_executable(ChatClient_main ChatClient_main.cc ChatClient.cc ../codec.cc ${KANON_SRC})
