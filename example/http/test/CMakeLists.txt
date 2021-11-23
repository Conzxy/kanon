add_executable(HttpRequestParser_test HttpRequestParser_test.cc ../HttpRequestParser.cc ${STRING_SRC})
target_link_libraries(HttpRequestParser_test gtest)

add_executable(HttpResponse_test HttpResponse_test.cc ../HttpResponse.cc ../HttpGlobal.cc ${STRING_SRC})
