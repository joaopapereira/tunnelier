CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(tunnelier_src tunnelier.cpp requestHandler.cpp SharedMemory.cpp) 
ADD_EXECUTABLE( tunnelier ${tunnelier_src})
message("Libs in the target")
message("${LIBS}")
TARGET_LINK_LIBRARIES(tunnelier ${ADDITIONAL_LINK_LIBS} pthread event event_pthreads jsoncpp ssh JPLoggerStatic tunnel ${LIBS})
