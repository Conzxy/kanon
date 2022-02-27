add_executable(thread_pool_test thread_pool_test.cc)
add_executable(thread_test thread_test.cc)

target_link_libraries(thread_test kanon_base)
target_link_libraries(thread_pool_test kanon_base)
#add_executable(destruct1 destruct1.cc ${KANON_SRC})
#add_executable(object_poll_test object-pool-test.cc ${KANON_SRC})
