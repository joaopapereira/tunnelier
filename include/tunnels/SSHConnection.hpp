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

class SSHConnection {
public:
	SSHConnection(Address host, User user);
	virtual ~SSHConnection();
	int connect();
	int authenticate_kbdint();
	SSHRemoteEndPoint * createEndPoint(Address destination, struct event_base * workerEventBase);
private:
	Address host;
	User user;
	ssh_session connection;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* SSHCONNECTION_HPP_ */
