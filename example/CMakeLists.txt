set(EXAMPLE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/example)

function (GenExample exec_name)
  if (KANON_BUILD_ALL_EXAMPLES)   
    add_executable(${exec_name} ${ARGN})
  else ()
    message(STATUS "Example target [${exec_name}] is excluded from target all")
    add_executable(${exec_name} EXCLUDE_FROM_ALL ${ARGN})
  endif ()   
  
  target_link_libraries(${exec_name} kanon_net kanon_base)
  set_target_properties(${exec_name}
    PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/example
  )
endfunction ()

add_subdirectory(daytime)
add_subdirectory(echo)
add_subdirectory(discard)
add_subdirectory(chat)
add_subdirectory(file_transfer)
