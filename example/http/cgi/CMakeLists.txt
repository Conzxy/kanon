add_executable(adder adder.cc cgi_parse.cc ${HTTP_SRC})
target_link_libraries(adder kanon_base kanon_net)

set_target_properties(adder PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(test)