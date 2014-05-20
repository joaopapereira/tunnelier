/*
 * SSHRemoteEndPoint.hpp
 *
 *  Created on: Apr 16, 2014
 *      Author: joao
 */

#ifndef SSHREMOTEENDPOINT_HPP_
#define SSHREMOTEENDPOINT_HPP_

#include <tunnels/EndPoint.hpp>
#include <Address.hpp>
#include <User.hpp>
#include "libJPLogger.hpp"
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <mutex>
#include <event.h>
#include <iostream>
namespace tunnelier {
namespace tunnels {

class SSHRemoteEndPoint: public EndPoint {
public:
	SSHRemoteEndPoint(Address middleHost, User middleUser, Address destination);
	SSHRemoteEndPoint(Address middleHost, User middleUser, Address destination, ssh_channel channel, struct event_base * workerEventBase);
	virtual ~SSHRemoteEndPoint();
	virtual int poll();
	inline bool operator==(SSHRemoteEndPoint &rhs){
		return (middleHost == rhs.middleHost) &&
			   (middleUser == rhs.middleUser) &&
			   (address    == rhs.address);
	}
	inline void setReadCallBack(event_callback_fn callBack, void * arguments){
		std::lock_guard<std::mutex> lock(mutex);
		readCallBack = callBack;
		event_assign(socket_event, workerEventBase, -1, EV_WRITE, callBack, arguments);
		this->argument = arguments;
	}
	inline void setReadCallback(ssh_channel_data_callback callback, void*data){


		channel_cb.channel_data_function = callback;
		channel_cb.userdata = data;
		ssh_callbacks_init(&channel_cb);
		ssh_set_channel_callbacks(channel, &channel_cb);
	}
	virtual int writeToEndPoint( void* data, int length);
	int writeToSocket(int socket_fd);

private:
	virtual int readFromEndPoint();
	Address address;
	Address middleHost;
	User middleUser;
	ssh_channel channel;
	std::mutex mutex;
	struct ssh_channel_callbacks_struct channel_cb;
	
	event_callback_fn readCallBack;
	struct event * socket_event;
	struct event_base * workerEventBase;
	void * argument;
	std::string LOGNAME;


};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SSHREMOTEENDPOINT_HPP_ */
