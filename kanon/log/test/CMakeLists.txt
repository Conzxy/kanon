add_executable(Logger_test Logger_test.cc ${KANON_BASE_SRC})

add_executable(LogFile_test LogFile_test.cc ${KANON_BASE_SRC})
add_executable(AsyncLog_test AsyncLog_test.cc ${KANON_BASE_SRC})

add_executable(LogFile_bench LogFile_bench.cc ${KANON_BASE_SRC})
target_link_libraries(LogFile_bench benchmark)

add_executable(AsyncLog_bench AsyncLog_bench.cc ${KANON_BASE_SRC})
