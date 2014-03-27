CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(tunnelier_src tunnelier.cpp) 
ADD_EXECUTABLE( tunnelier ${tunnelier_src})
TARGET_LINK_LIBRARIES(tunnelier ${LIBS} event)

