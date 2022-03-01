add_executable(
  http_request_parser_test http_request_parser_test.cc 
  ${HTTP_SRC_DIR}/http_request_parser.cc )

target_link_libraries(http_request_parser_test gtest kanon_base)

add_executable(
  http_response_test http_response_test.cc 
  ${HTTP_SRC} ${STRING_SRC})

add_executable(
  file_test file_test.cc 
  ${HTTP_SRC_DIR}/file.cc)

add_executable(
  process_test process_test.cc 
  ${HTTP_SRC_DIR}/process.cc)

add_executable(
  pipe_test pipe_test.cc 
  ${HTTP_SRC_DIR}/process.cc ${HTTP_SRC_DIR}/pipe.cc)