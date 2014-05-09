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
#include <libssh/libssh.h>
#include <event.h>
namespace tunnelier {
namespace tunnels {
const int MAX_NUMBER_CHANNELS = 2;
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
		evutil_make_socket_nonblocking(ssh_get_fd(connection));
		if(nullptr == channel_to_socket_event)
			channel_to_socket_event = event_new( base, ssh_get_fd(connection), EV_READ, callBack, arguments);
		else
			event_assign(channel_to_socket_event, base, ssh_get_fd(connection), EV_READ, callBack, arguments);
		callBackArgument = arguments;
		event_add(channel_to_socket_event, nullptr);
		std::cout << "Setting poll callback: " << ssh_get_fd(connection)<< std::endl;
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
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SSHCONNECTION_HPP_ */
