set(THREAD_TEST_NEED ${ALL_SRC})
list(REMOVE_ITEM THREAD_TEST_NEED ${NET_SRC})


message(STATUS "THREAD_TEST_NEED: ${THREAD_TEST_NEED}")

add_executable(ThreadPool_test ThreadPool_test.cc ${THREAD_TEST_NEED})
#add_executable(thread_test thread-test.cc ${THREAD_TEST_NEED})
#add_executable(destruct1 destruct1.cc ${KANON_SRC})
#add_executable(object_poll_test object-pool-test.cc ${KANON_SRC})
