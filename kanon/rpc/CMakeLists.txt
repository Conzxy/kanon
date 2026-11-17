add_custom_command(
  OUTPUT rpc.pb.h rpc.pb.cc
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

aux_source_directory(. KANON_PROTOBUF_RPC_SRC)
list(APPEND KANON_PROTOBUF_RPC_SRC rpc.pb.cc)

kanon_gen_lib(kanon_protobuf_rpc ${KANON_PROTOBUF_RPC_SRC})
target_include_directories(kanon_protobuf_rpc PRIVATE ${PROJECT_BINARY_DIR}) # i.e. kanon/build
target_link_libraries(kanon_protobuf_rpc PUBLIC kanon_protobuf)

file(GLOB KANON_PROTOBUF_RPC_INCLUDE "./*.h")
list(APPEND RPC_INCLUDE ${PROJECT_BINARY_DIR}/kanon/rpc/rpc.pb.h)

if (KANON_INSTALL)
  install(FILES ${KANON_PROTOBUF_RPC_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/rpc)
endif ()
