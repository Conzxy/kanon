add_custom_command(
  OUTPUT rpc.pb.h rpc.pb.cc
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

include_directories(${PROJECT_BINARY_DIR}) # i.e. kanon/build

add_library(protobuf_rpc rpc.pb.cc krpc_channel.cc krpc_server.cc)
target_link_libraries(protobuf_rpc protobuf_codec)

add_subdirectory(test)
