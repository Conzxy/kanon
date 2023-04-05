# In other platform(eg. windows), generating base library only.
if (UNIX AND NOT APPLE)
  message(STATUS "Kanon on unix/linux/solaris/... platform")
  set(KANON_ON_UNIX ON)
else ()
  set(KANON_ON_UNIX OFF)
endif ()

if (WIN32)
  message(STATUS "Kanon on windows")
  set(KANON_ON_WIN ON)
endif ()

