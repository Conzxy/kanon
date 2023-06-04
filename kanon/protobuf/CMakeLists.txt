kanon_gen_lib(kanon_protobuf generic_pb_codec.cc logger.cc protobuf_codec2.cc chunk_stream.cc kvarint/kvarint.c)

if (KANON_LINK_PROTOBUF)
  find_package(Protobuf REQUIRED)
  target_link_libraries(kanon_protobuf PUBLIC protobuf::libprotobuf)
else ()
  if ("${kanon_protobuf_to_link}" STREQUAL "")
    message(FATAL_ERROR "User must set kanon_protobuf_to_link variable: user specified protobuf lib")
  endif ()
  target_link_libraries(kanon_protobuf PUBLIC ${kanon_protobuf_to_link})
endif ()

target_link_libraries(kanon_protobuf PUBLIC kanon_net)

file(GLOB KANON_PROTOBUF_INCLUDE "*.h")

if (KANON_INSTALL)
  install(FILES ${KANON_PROTOBUF_INCLUDE} DESTINATION "${KANON_INSTALL_INCLUDE_PREFIX}/protobuf")
endif ()