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
	friend std::ostream& operator<<(std::ostream& os, const SSHConnection& con)
	{
	  os << con.user << "@" << con.host << ": " << con.num_channels << "/" << MAX_NUMBER_CHANNELS << " Currently active: " << con.num_active_channels;
	  return os;
	};
private:
	Address host;
	User user;
	ssh_session connection;
	int num_channels;
	int num_active_channels;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SSHCONNECTION_HPP_ */
