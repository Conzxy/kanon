add_subdirectory(test)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} HTTP_SRC)

message(STATUS "${HTTP_SRC}")
add_executable(HttpServerMain HttpServerMain.cc ${HTTP_SRC} ${KANON_SRC})
