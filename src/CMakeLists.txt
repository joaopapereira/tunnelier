CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(tunnelier_src tunnelier.cpp requestHandler.cpp SharedMemory.cpp) 
ADD_EXECUTABLE( tunnelier ${tunnelier_src})
TARGET_LINK_LIBRARIES(tunnelier ${LIBS} event jsoncpp tunnel)

