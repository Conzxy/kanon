# The xxHash don't install when it is a subdirectory default.
#add_subdirectory(xxHash/cmake_unofficial)

# Required by leak_detector implementation
# Don't install any files related libgc
# Exclude this from default build target but
# if project dependent some targets of this,
# only these targets will be built and used.
if (KANON_TESTS)
  add_subdirectory(bdwgc EXCLUDE_FROM_ALL)
endif ()
