#set(XXHASH_DIR ${THIRD_PARTY_DIR}/xxHash)
#file(GLOB XXHASH_SRC ${XXHASH_DIR}/*.c)
#GenLib(protobuf_codec generic_pb_codec.cc chunk_stream.cc ${XXHASH_SRC})
GenLib(protobuf_codec generic_pb_codec.cc chunk_stream.cc)
# target_include_directories(protobuf_codec PRIVATE ${THIRD_PARTY_DIR})
target_link_libraries(protobuf_codec protobuf kanon_base kanon_net)

install(TARGETS protobuf_codec DESTINATION lib)

file(GLOB PROTOBUF_INCLUDE ./*.h)

message("protobuf codec include: ${PROTOBUF_INCLUDE}")
install(FILES ${PROTOBUF_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/protobuf)
