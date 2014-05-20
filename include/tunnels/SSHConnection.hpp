/*
 * SSHConnection.hpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
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
class SSHConnection {
public:
	SSHConnection(Address host, User user);
	SSHConnection(SSHConnection& original);
	virtual ~SSHConnection();
	int connect();
	int authenticate_kbdint();
	SSHRemoteEndPoint * createEndPoint(Address destination, struct event_base * workerEventBase);
	inline bool canCreateChannel(){
		return MAX_NUMBER_CHANNELS > num_channels;
	};
	inline bool canBeRemoved(){
		return num_active_channels < 1 && !canCreateChannel();
	};
	inline void deactivatedChannel(){
		num_active_channels--;
		if( 0 == num_active_channels )
			event_del(channel_to_socket_event);
	};
	inline socket_t getConnectionSocket(){
		return ssh_get_fd(connection);
	};
	friend std::ostream& operator<<(std::ostream& os, const SSHConnection& con)
	{
	  os << con.user << "@" << con.host << ": " << con.num_channels << "/" << MAX_NUMBER_CHANNELS << " Currently active: " << con.num_active_channels;
	  return os;
	};
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
	Address host;
	User user;
	ssh_session connection;
	int num_channels;
	int num_active_channels;

	struct event *channel_to_socket_event;
	event_callback_fn pollCallback;
	void * callBackArgument;
	std::string LOGNAME;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SSHCONNECTION_HPP_ */
