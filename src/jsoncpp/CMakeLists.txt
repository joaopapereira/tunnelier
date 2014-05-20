CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET( jsoncpp_lib_srcs jsoncpp.cpp )
include_directories( include/ ${INSTALL_INCS})
ADD_LIBRARY( jsoncpp STATIC ${jsoncpp_lib_srcs})
