CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

include_directories( include/ ${INSTALL_INCS})
SET(tunnel_lib_srcs  EndPoint.cpp TunnelManager.cpp TunnelWorker.cpp EventAcceptor.cpp SSHConnection.cpp SocketListener.cpp SSHRemoteEndPoint.cpp LocalSocket.cpp)
ADD_LIBRARY( tunnel STATIC ${tunnel_lib_srcs})

