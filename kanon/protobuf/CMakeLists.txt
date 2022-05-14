GenLib(protobuf_codec generic_pb_codec.cc)
target_link_libraries(protobuf_codec protobuf z)

install(TARGETS protobuf_codec DESTINATION lib)

file(GLOB PROTOBUF_INCLUDE ./*.h)

message("protobuf codec include: ${PROTOBUF_INCLUDE}")
install(FILES ${PROTOBUF_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/protobuf)