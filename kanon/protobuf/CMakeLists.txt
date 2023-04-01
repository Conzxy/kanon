GenLib(kanon_protobuf generic_pb_codec.cc chunk_stream.cc)
target_link_libraries(kanon_protobuf PUBLIC protobuf::libprotobuf PUBLIC kanon_net)

file(GLOB KANON_PROTOBUF_INCLUDE "*.h")

if (KANON_INSTALL)
  install(FILES ${KANON_PROTOBUF_INCLUDE} DESTINATION "${KANON_INSTALL_INCLUDE_PREFIX}/protobuf")
endif ()
