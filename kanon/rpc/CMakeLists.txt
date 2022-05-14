add_custom_command(
  OUTPUT rpc.pb.h rpc.pb.cc
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

include_directories(${PROJECT_BINARY_DIR}) # i.e. kanon/build

aux_source_directory(. RPC_SRC)
list(APPEND RPC_SRC rpc.pb.cc)
message("rpc sources: ${RPC_SRC}")

GenLib(protobuf_rpc ${RPC_SRC})

target_link_libraries(protobuf_rpc protobuf_codec)
install(TARGETS protobuf_rpc DESTINATION lib)

file(GLOB RPC_INCLUDE ./*.h)
list(APPEND RPC_INCLUDE ${PROJECT_BINARY_DIR}/kanon/rpc/rpc.pb.h)

message("rpc header files: ${RPC_INCLUDE}")
install(FILES ${RPC_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/rpc)