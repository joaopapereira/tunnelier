/* SSHRemoteEndPoint.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef SSHREMOTEENDPOINT_HPP_
#define SSHREMOTEENDPOINT_HPP_

#include "libJPLogger.hpp"
#include "cxx11_implementations.hpp"
#include <tunnels/EndPoint.hpp>
#include <Address.hpp>
#include <User.hpp>
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <event.h>
#include <iostream>
namespace tunnelier {
namespace tunnels {
/**
 * Class that hold the remote connection
 */
class SSHRemoteEndPoint: public EndPoint {
public:
	/**
	 * Class constructor
	 * @param middleHost Middle host information
	 * @param middleUser Middle host user
	 * @param destination Destination of the tunnel
	 */
	SSHRemoteEndPoint(Address middleHost, User middleUser, Address destination);
	/**
	 * Class constructor
	 * @param middleHost Middle host information
	 * @param middleUser Middle host user
	 * @param destination Destination of the tunnel
	 * @param channel SSH Channel
	 * @param workerEventBase Worker event base
	 */
	SSHRemoteEndPoint(Address middleHost, User middleUser, Address destination, ssh_channel channel, struct event_base * workerEventBase);
	/**
	 * Class destructor
	 */
	virtual ~SSHRemoteEndPoint();
	/**
	 * Polling function in the remote end point
	 * @return Amount of data in the socket
	 */
	virtual int poll();
	/**
	 * Overload of comparison operator
	 * @param rhs Other End point to compare to
	 */
	inline bool operator==(SSHRemoteEndPoint &rhs){
		return (middleHost == rhs.middleHost) &&
			   (middleUser == rhs.middleUser) &&
			   (address    == rhs.address);
	}
	/**
	 * Set the read call back on the channel
	 * @param callBack Function to be called
	 * @param arguments Arguments to be passed to the call back
	 */
	inline void setReadCallBack(event_callback_fn callBack, void * arguments){
#ifdef USE_BOOST_INSTEAD_CXX11
		std::lock_guard<std::mutex*> lock(&mutex);
#else
		std::lock_guard<std::mutex> lock(mutex);
#endif
		readCallBack = callBack;
		event_assign(socket_event, workerEventBase, -1, EV_WRITE, callBack, arguments);
		this->argument = arguments;
	}
	/**
	 * Set the read call back on the channel
	 * Using libssh version
	 * @param callback Function to be called
	 * @param arguments Arguments to be passed to the call back
	 */
	inline void setReadCallback(ssh_channel_data_callback callback, void*data){


		channel_cb.channel_data_function = callback;
		channel_cb.userdata = data;
		ssh_callbacks_init(&channel_cb);
		ssh_set_channel_callbacks(channel, &channel_cb);
	}
	/**
	 * Write to the current end point
	 * @param data Data to be written
	 * @param length amount of data to be written
	 * @return Amount of data written
	 */
	virtual int writeToEndPoint( void* data, int length);
	/**
	 * Write to the socket
	 * @param socket_fd Socket to write to
	 * @return Amount of data written
	 */
	int writeToSocket(int socket_fd);

private:
	/**
	 * Read from the end point
	 */
	virtual int readFromEndPoint();
	/**
	 * Destination address
	 */
	Address address;
	/**
	 * Middle host information
	 */
	Address middleHost;
	/**
	 * Middle host user
	 */
	User middleUser;
	/**
	 * SSH Channel
	 */
	ssh_channel channel;
	/**
	 * Mutex
	 */
	std::mutex mutex;
	/**
	 * Libssh call back structure
	 */
	struct ssh_channel_callbacks_struct channel_cb;
	/**
	 * Libevent read call back
	 */
	event_callback_fn readCallBack;
	/**
	 * Socket event
	 */
	struct event * socket_event;
	/**
	 * Worker event base
	 */
	struct event_base * workerEventBase;
	/**
	 * Arguments to be passed to the call back
	 */
	void * argument;
	/**
	 * Log name
	 */
	std::string LOGNAME;


};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SSHREMOTEENDPOINT_HPP_ */
