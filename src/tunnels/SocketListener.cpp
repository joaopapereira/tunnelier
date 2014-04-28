/*
 * SocketListener.cpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#include "tunnels/SocketListener.hpp"

namespace tunnelier {
namespace tunnels {

SocketListener::SocketListener(struct event_base* base, int localPort):
	EventAcceptor(base),
	localPort(localPort),
	listener(nullptr),
	accept_callback(nullptr),
	error_callback(nullptr){
}
int SocketListener::bind(void * attributes){
	// TODO Auto-generated constructor stub
	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(localPort);
	listener = evconnlistener_new_bind(base, accept_callback, attributes,
		LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
		(struct sockaddr*)&sin, sizeof(sin));
	if (!listener) {
			perror("Couldn't create listener");
			return -1;
	}
	evconnlistener_set_error_cb(listener, error_callback);
	return 0;
}


SocketListener::~SocketListener() {
	// TODO Auto-generated destructor stub
}

} /* namespace tunnels */
} /* namespace tunnelier */
