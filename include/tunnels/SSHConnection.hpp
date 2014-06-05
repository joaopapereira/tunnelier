/* LocalSocket.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef SSHCONNECTION_HPP_
#define SSHCONNECTION_HPP_
#include "Address.hpp"
#include "User.hpp"
#include "SSHRemoteEndPoint.hpp"
#include "libJPLogger.hpp"
#include <libssh/libssh.h>
#include <event.h>
#include <unistd.h>
#include <fcntl.h>
namespace tunnelier {
namespace tunnels {
const int MAX_NUMBER_CHANNELS = 15;
/**
 * SSH Connection handler
 */
class SSHConnection {
public:
	/**
	 * Class constructor
	 * @param host Host information
	 * @param user User information
	 */
	SSHConnection(Address host, User user);
	/**
	 * Copy constructor
	 * @param original Connection to copy information from
	 */
	SSHConnection(SSHConnection& original);
	/**
	 * Class destructor
	 */
	virtual ~SSHConnection();
	/**
	 * Function that connect to the host
	 * @return 0 in case of success
	 */
	int connect();
	/**
	 * Function that do a keyboard interactive authentication
	 * @return 0 in case of success
	 */
	int authenticate_kbdint();
	/**
	 * Creates a forwarding port to a destination
	 * @param destination Host information
	 * @param workerEventBase Worker base to set the connection to
	 */
	SSHRemoteEndPoint * createEndPoint(Address destination, struct event_base * workerEventBase);
	/**
	 * Check if a new channel can be created or a new connection is needed
	 * @return true if a new channel is supported
	 */
	inline bool canCreateChannel(){
		return MAX_NUMBER_CHANNELS > num_channels;
	};
	/**
	 * Check if the connection can be removed
	 * @return true if can be removed
	 */
	inline bool canBeRemoved(){
		return num_active_channels < 1 && !canCreateChannel();
	};
	/**
	 * Remove one channel from the active list
	 */
	inline void deactivatedChannel(){
		num_active_channels--;
		if( 0 == num_active_channels )
			event_del(channel_to_socket_event);
	};
	/**
	 * Retrieve the current socket
	 * @return socket_t
	 */
	inline socket_t getConnectionSocket(){
		return ssh_get_fd(connection);
	};
	/**
	 * Overload operator <<
	 * @param os Output Stream
	 * @param con Connection to be written
	 */
	friend std::ostream& operator<<(std::ostream& os, const SSHConnection& con)
	{
	  os << con.user << "@" << con.host << ": " << con.num_channels << "/" << MAX_NUMBER_CHANNELS << " Currently active: " << con.num_active_channels;
	  return os;
	};
	/**
	 * Set the polling of the current connection socket
	 * @param callBack Function to be called when something need to be readed in the socket
	 * @param arguments Arguments to be passed to the call back
	 * @param event_base Base where the call back will be called
	 */
	inline void setPollCallBack(event_callback_fn callBack, void * arguments, struct event_base * base){
		pollCallback = callBack;
		socket_t newSock =dup(ssh_get_fd(connection));
		fcntl (newSock, F_SETFD, fcntl (newSock, F_GETFD) | FD_CLOEXEC);
		evutil_make_socket_nonblocking(newSock);
		if(nullptr == channel_to_socket_event)
			channel_to_socket_event = event_new( base, newSock, EV_READ|EV_PERSIST, callBack, arguments);
		else
			event_assign(channel_to_socket_event, base, newSock, EV_READ|EV_PERSIST, callBack, arguments);
		callBackArgument = arguments;
	}
private:
	/**
	 * Destination host information
	 */
	Address host;
	/**
	 * User information
	 */
	User user;
	/**
	 * SSH Connection
	 */
	ssh_session connection;
	/**
	 * Number of channels used in the current connection
	 */
	int num_channels;
	/**
	 * Number of channels currently open
	 */
	int num_active_channels;
	/**
	 * Event to transport information from the channel to the local socket
	 */
	struct event *channel_to_socket_event;
	/**
	 * Pool event callback
	 */
	event_callback_fn pollCallback;
	/**
	 * Information to be passed into the callback
	 */
	void * callBackArgument;
	/**
	 * Log name
	 */
	std::string LOGNAME;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SSHCONNECTION_HPP_ */
