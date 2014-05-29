/*
 * SocketListener.hpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#ifndef SOCKETLISTENER_HPP_
#define SOCKETLISTENER_HPP_

#include "tunnels/EventAcceptor.hpp"
#include "cxx11_implementations.hpp"
#include <event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>


namespace tunnelier {
namespace tunnels {

class SocketListener: public EventAcceptor {
public:
	SocketListener(struct event_base *base, int localPort);
	int bind(void * manager);
	virtual ~SocketListener();
	void setAcceptCallBack(evconnlistener_cb accept_callback){
		this->accept_callback = accept_callback;
	};
	void setErrorCallBack(evconnlistener_errorcb error_callback){
		this->error_callback = error_callback;
	};
	inline int getLocalPort(){
		return localPort;
	};
private:
	int localPort;
	struct evconnlistener *listener;
	evconnlistener_cb accept_callback;
	evconnlistener_errorcb error_callback;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SOCKETLISTENER_HPP_ */
