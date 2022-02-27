add_executable(lexical-stream-test lexical-stream-test.cc ${STRING_SRC_LIST})
target_link_libraries(lexical-stream-test gtest)

#add_executable(string-builder-test string-builder-test.cc ${STRING_SRC_LIST})

add_executable(string-view-test string-view-test.cc ${STRING_SRC})
target_link_libraries(string-view-test gtest)

add_executable(lexical-cast-test lexical-cast-test.cc ${STRING_SRC_LIST})
target_link_libraries(lexical-cast-test gtest)

add_executable(fmt_stream_test fmt_stream_test.cc ${STRING_SRC})
target_link_libraries(fmt_stream_test gtest)

add_executable(stream_bench stream_bench.cc ${STRING_SRC})
target_link_libraries(stream_bench gtest)