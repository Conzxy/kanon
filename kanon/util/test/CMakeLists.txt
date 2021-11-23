add_executable(check-test check-test.cc ${STRING_SRC_LIST})
add_executable(Any_test Any_test.cc)

target_link_libraries(Any_test gtest)
