# In other platform(eg. windows), generating base library only.
if (UNIX AND NOT APPLE)
  message(STATUS "Kanon on unix/linux/solaris/... platform")
  set(KANON_ON_UNIX 1)
else ()
  set(AKNON_ON_UNIX 0)
endif ()

if (WIN32)
  message(STATUS "Kanon on windows")
endif ()

