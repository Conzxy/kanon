GenLib(kanon_protobuf generic_pb_codec.cc logger.cc protobuf_codec2.cc chunk_stream.cc kvarint/kvarint.c)
target_link_libraries(kanon_protobuf PUBLIC protobuf::libprotobuf PUBLIC kanon_net)

file(GLOB KANON_PROTOBUF_INCLUDE "*.h")

if (KANON_INSTALL)
  install(FILES ${KANON_PROTOBUF_INCLUDE} DESTINATION "${KANON_INSTALL_INCLUDE_PREFIX}/protobuf")
endif ()
