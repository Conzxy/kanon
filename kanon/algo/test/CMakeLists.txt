add_executable(RingBuffer_test RingBuffer_test.cc ${KANON_SRC})
add_executable(BoundedQueue_test BoundedQueue_test.cc ${KANON_SRC})
target_link_libraries(BoundedQueue_test gtest)