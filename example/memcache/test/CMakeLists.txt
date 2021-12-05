add_executable(Tokenizer_test Tokenizer_test.cc ${STRING_SRC})


add_executable(Extractor_test Extractor_test.cc ${STRING_SRC})
target_link_libraries(Extractor_test gtest)
