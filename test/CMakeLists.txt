enable_testing()
file(GLOB KANON_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*test*.cc")
file(GLOB KANON_BENCH_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*bench*.cc")

#list(APPEND KANON_TEST_BENCH_SOURCES ${KANON_TEST_SOURCES} ${KANON_BENCH_SOURCES})
#message(STATUS "kanon test source files: ${KANON_TEST_SOURCES}")
#message(STATUS "kanon bench source files: ${KANON_BENCH_SOURCES}")

############################################
# generate test and bench program
############################################
macro (GenTest SRC)
  foreach (kanon_test_source ${${SRC}})
    get_filename_component(kanon_test_filename ${kanon_test_source} NAME_WE)

    if (KANON_BUILD_ALL_TESTS)
      add_executable(${kanon_test_filename} ${kanon_test_source})
    else ()
      message(STATUS "test target [${kanon_test_filename}] is excluded from target all")
      add_executable(${kanon_test_filename} EXCLUDE_FROM_ALL ${kanon_test_source})
    endif ()
  
    target_link_libraries(${kanon_test_filename} ${ARGN})
    target_include_directories(${kanon_test_filename} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    set_target_properties(${kanon_test_filename}
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
      COMMAND ${kanon_test_filename}
    )

    add_test(NAME ${kanon_test_filename} COMMAND ${kanon_test_filename})
  endforeach (kanon_test_source ${KANON_TEST_SOURCES})
endmacro()

GenTest(KANON_TEST_SOURCES kanon_base kanon_net gtest gtest_main gc)
GenTest(KANON_BENCH_SOURCES kanon_base kanon_net benchmark benchmark_main gtest gtest_main gc)

if (KANON_BUILD_PROTOBUF)
  add_subdirectory(rpc)
endif ()
if (KANON_BUILD_PROTOBUF_RPC)
  add_subdirectory(protobuf)
endif ()

if (KANON_BUILD_PERF_TEST)
  add_subdirectory(perf)
endif ()
