/*
 * LocalSocket.hpp
 *
 *  Created on: Apr 10, 2014
 *      Author: joao
 */

#ifndef LOCALSOCKET_HPP_
#define LOCALSOCKET_HPP_
#include "EndPoint.hpp"
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

namespace tunnelier {
namespace tunnels {
#define USE_UNIX_SOCKET 0
class LocalSocket: public EndPoint {
public:
	LocalSocket(int socket_fd, struct event_base * workerEventBase);
	void bindSocket(void * arguments);
	virtual ~LocalSocket();
	int poll();
#if !USE_UNIX_SOCKET
	inline void setReadCallBack(bufferevent_data_cb callBack){
#else
	inline void setReadCallBack(event_callback_fn callBack){
#endif
		readCallBack = callBack;
	}
	inline void setErrorCallBack(bufferevent_event_cb callBack){
		errorCallBack = callBack;
	}
	int writeToEndPoint(void* data, int length);
	int getSocketId(){return socket_id;};
	struct event_base* getWorkerEventBase(){
		return workerEventBase;
	};

private:
	int readFromEndPoint(){ return 0;};
#if !USE_UNIX_SOCKET
	bufferevent_data_cb readCallBack;
#else
	event_callback_fn readCallBack;
#endif
	bufferevent_event_cb errorCallBack;
	/*static void socket_to_ssh(struct bufferevent *bev, void *ctx);
	static void errorcb(struct bufferevent *bev, short events, void *ctx);*/
#if !USE_UNIX_SOCKET
	bufferevent * socket_event;
#else
	struct event * socket_event;
#endif
	int socket_id;
	struct event_base * workerEventBase;
};

} /* namespace tunnels */
} /* namespace tunnelier */


#endif /* LOCALSOCKET_HPP_ */
