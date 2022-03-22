set(BUILD_ALL_EXAMPLES OFF CACHE BOOL "Determine if build all examples")


function (GenExample exec_name)
  if (${BUILD_ALL_EXAMPLES})   
    add_executable(${exec_name} ${ARGN})
  else ()
    add_executable(${exec_name} EXCLUDE_FROM_ALL ${ARGN})
  endif (${BUILD_ALL_EXAMPLES})   
  
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