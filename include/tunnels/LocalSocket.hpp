/* LocalSocket.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef LOCALSOCKET_HPP_
#define LOCALSOCKET_HPP_
#include "EndPoint.hpp"
#include "libJPLogger.hpp"
#include "cxx11_implementations.hpp"
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

namespace tunnelier {
namespace tunnels {
#define USE_UNIX_SOCKET 0
/**
 * Class that holds the local socket in a Tunnel
 */
class LocalSocket: public EndPoint {
public:
	/**
	 * Class constructor
	 * @param socket_fd Socket file descriptor
	 * @param workerEventBase Event Base
	 */
	LocalSocket(int socket_fd, struct event_base * workerEventBase);
	/**
	 * Bind the socket with the event that need to be fired
	 * @param arguments Arguments that will be passed to the function that
	 * 					manages the event
	 */
	void bindSocket(void * arguments);
	/**
	 * Class descrutor
	 */
	virtual ~LocalSocket();
	/**
	 * Do a poll in the socket to see if it as anything to read
	 * @return Amount of data currently in the socket
	 */
	int poll();
#if !USE_UNIX_SOCKET
	/**
	 * Set the callback that will be used when the event gets triggered
	 * @param callBack pointer to the function to be called
	 */
	inline void setReadCallBack(bufferevent_data_cb callBack){
#else

	inline void setReadCallBack(event_callback_fn callBack){
#endif
		readCallBack = callBack;
	}
	/**
	 * Set the callback that will be used when the event gets triggered when error happens
	 * @param callBack pointer to the function to be called
	 */
	inline void setErrorCallBack(bufferevent_event_cb callBack){
		errorCallBack = callBack;
	}
	/**
	 * Function to write to the current endpoit
	 * @param data Data to be written
	 * @param length AMount of data to write
	 * @return Amount of data written
	 */
	int writeToEndPoint(void* data, int length);
	/**
	 * Retrieve the socket id
	 * @return Socket ID
	 */
	int getSocketId(){return socket_id;};
	/**
	 * Retrieve the worker event base used by this socket
	 */
	struct event_base* getWorkerEventBase(){
		return workerEventBase;
	};

private:
	/**
	 * Read from socket
	 * @deprecated
	 */
	int readFromEndPoint(){ return 0;};
#if !USE_UNIX_SOCKET
	/**
	 * Event callback for read
	 */
	bufferevent_data_cb readCallBack;
#else

	event_callback_fn readCallBack;
#endif
	/**
	 * Event callback for erros
	 */
	bufferevent_event_cb errorCallBack;
	/*static void socket_to_ssh(struct bufferevent *bev, void *ctx);
	static void errorcb(struct bufferevent *bev, short events, void *ctx);*/
#if !USE_UNIX_SOCKET
	/**
	 * Socket event
	 */
	bufferevent * socket_event;
#else

	struct event * socket_event;
#endif
	/**
	 * Socket ID
	 */
	int socket_id;
	/**
	 * Event base
	 */
	struct event_base * workerEventBase;
	/**
	 * Log name
	 */
	std::string LOGNAME;
};

} /* namespace tunnels */
} /* namespace tunnelier */


#endif /* LOCALSOCKET_HPP_ */
