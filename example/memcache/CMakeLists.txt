aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} MEMCACHE_SRC)

add_executable(MemcacheServer_main ${MEMCACHE_SRC} ${KANON_SRC})
target_link_libraries(MemcacheServer_main boost_program_options)

add_subdirectory(test)