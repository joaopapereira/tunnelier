/* LocalSocket.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
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
/**
 * Local listener
 * This class waits for connection in the localhost
 */
class SocketListener: public EventAcceptor {
public:
	/**
	 * Class constructor
	 * @param base Event base
	 * @param localPort Local port to listen at
	 */
	SocketListener(struct event_base *base, int localPort);
	/**
	 * Bind the socket to a event
	 * @param manager Parameter to be passed in the callbacks
	 */
	int bind(void * manager);
	/**
	 * Class destructor
	 */
	virtual ~SocketListener();
	/**
	 * Set the Accept Call Back
	 * @param accept_callback Call back to be used when accepting connection
	 */
	void setAcceptCallBack(evconnlistener_cb accept_callback){
		this->accept_callback = accept_callback;
	};
	/**
	 * Set the Error Call Back
	 * @param error_callback Call back to be used when and error happens
	 */
	void setErrorCallBack(evconnlistener_errorcb error_callback){
		this->error_callback = error_callback;
	};
	/**
	 * Retrieve the port where this object is listening
	 * @return Port number
	 */
	inline int getLocalPort(){
		return localPort;
	};
private:
	/**
	 * Port number
	 */
	int localPort;
	/**
	 * Port listener
	 */
	struct evconnlistener *listener;
	/**
	 * Accept callback
	 */
	evconnlistener_cb accept_callback;
	/**
	 * Error callback
	 */
	evconnlistener_errorcb error_callback;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SOCKETLISTENER_HPP_ */
