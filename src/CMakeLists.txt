CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(tunnelier_src tunnelier.cpp requestHandler.cpp SharedMemory.cpp) 
ADD_EXECUTABLE( tunnelier ${tunnelier_src})
message("Libs in the target")
message("${LIBS}")
TARGET_LINK_LIBRARIES(tunnelier event jsoncpp tunnel ${LIBS})
