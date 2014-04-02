CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

include_directories( include/ ${INSTALL_INCS})
SET(tunnel_lib_srcs Tunnel.cpp)
ADD_LIBRARY( tunnel SHARED ${tunnel_lib_srcs})

