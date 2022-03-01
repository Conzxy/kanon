set(HTTP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src HTTP_SRC)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

message(STATUS "HTTP_SRC: ${HTTP_SRC}")
# add_executable(HttpServerMain HttpServerMain.cc ${HTTP_SRC} ${KANON_SRC})
add_executable(http_server main.cc ${HTTP_SRC})
target_link_libraries(http_server kanon_base kanon_net)

add_subdirectory(test)
add_subdirectory(cgi)