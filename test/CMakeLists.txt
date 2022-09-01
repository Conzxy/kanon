#enable_testing()
file(GLOB KANON_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*test*.cc")
file(GLOB KANON_BENCH_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*bench*.cc")

#list(APPEND KANON_TEST_BENCH_SOURCES ${KANON_TEST_SOURCES} ${KANON_BENCH_SOURCES})

############################################
# User can determine whether to build all tests when build target all
# e.g. cmake --build */kanon/build [--target all -j 2]
############################################
set(BUILD_ALL_TESTS OFF CACHE BOOL "Don't exclude tests from target all")

message(STATUS "kanon test source files: ${KANON_TEST_SOURCES}")
message(STATUS "kanon bench source files: ${KANON_BENCH_SOURCES}")

if (NOT ${BUILD_ALL_TESTS})
  message(STATUS "[The kanon test and bench files are excluded from the build of target all]")
endif (NOT ${BUILD_ALL_TESTS})

############################################
# generate test and bench program
############################################
macro (GenTest SRC)
  foreach (kanon_test_source ${${SRC}})
    get_filename_component(kanon_test_filename ${kanon_test_source} NAME_WE)

    if (${BUILD_ALL_TESTS})
      add_executable(${kanon_test_filename} ${kanon_test_source})
    else ()
      message(STATUS "${kanon_test_filename} is excluded from target all")
      add_executable(${kanon_test_filename} EXCLUDE_FROM_ALL ${kanon_test_source})
    endif (${BUILD_ALL_TESTS})
  
    target_link_libraries(${kanon_test_filename} ${ARGN})

    set_target_properties(${kanon_test_filename}
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
      COMMAND ${kanon_test_filename}
    )

    add_test(NAME ${kanon_test_filename} COMMAND ${kanon_test_filename})
  endforeach (kanon_test_source ${KANON_TEST_SOURCES})
endmacro()

GenTest(KANON_TEST_SOURCES kanon_net kanon_base  gtest gtest_main)
GenTest(KANON_BENCH_SOURCES kanon_net kanon_base benchmark benchmark_main)
add_subdirectory(rpc)
add_subdirectory(protobuf)
