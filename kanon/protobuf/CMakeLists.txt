GenLib(protobuf_codec generic_pb_codec.cc)
target_link_libraries(protobuf_codec protobuf z)

install(TARGETS protobuf_codec DESTINATION lib)