/*
 * SSHConnection.cpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#include "tunnels/SSHConnection.hpp"
#include <iostream>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>

namespace tunnelier {
namespace tunnels {
using namespace std;
SSHConnection::SSHConnection(Address host, User user):
	host(host),
	user(user),
	connection(ssh_new()),
	num_channels(0),
	num_active_channels(0){
}
SSHConnection::SSHConnection(SSHConnection& original):
		host(original.host),
		user(original.user),
		connection(ssh_new()),
		num_channels(0),
		num_active_channels(0){
	int result = connect();
	if( 0 > result ){
		throw ios_base::failure("Unable to connect to the middle server!!");
	}
}
int
SSHConnection::connect() {
	std::cout << "Entered connect_tunnel"<<std::endl;
	int rc = 0;
	ssh_options_set(connection, SSH_OPTIONS_HOST, host.getHost().c_str());
	ssh_options_set(connection, SSH_OPTIONS_USER, user.getUsername().c_str());
	ssh_options_set(connection, SSH_OPTIONS_LOG_VERBOSITY_STR, "3");
	rc = ssh_connect(connection);
	if (rc != SSH_OK){
	  std::cout <<"Error connecting to " <<  host.getHost() << ": "<< ssh_get_error(connection)<<std::endl;
	  return -1;
	}
	std::cout <<"Trying to connect!!!!" << std::endl;
	rc = ssh_userauth_none(connection, NULL);
	if(rc == SSH_AUTH_SUCCESS){
	  std::cout << "Authenticated using method none" << std::endl;
	} else {
	  std::cout << "Authentication console with password: '"<<user.getPassword()<<"'"<<std::endl;
	  rc = authenticate_kbdint();
	  if(rc != SSH_AUTH_SUCCESS){
		  cerr << "Authentication failed: " << ssh_get_error(connection) << std::endl;
		  return -2;
	  }
	}

	return 0;
}

SSHConnection::~SSHConnection() {
	// TODO Auto-generated destructor stub
}
int
SSHConnection::authenticate_kbdint() {
     int err;

     err = ssh_userauth_kbdint(connection, NULL, NULL);
     while (err == SSH_AUTH_INFO) {
         const char *instruction;
         const char *name;
         char buffer[128];
         int i, n;

         name = ssh_userauth_kbdint_getname(connection);
         instruction = ssh_userauth_kbdint_getinstruction(connection);
         n = ssh_userauth_kbdint_getnprompts(connection);

         if (name && strlen(name) > 0) {
             std::cout <<name << std::endl;
         }

         if (instruction && strlen(instruction) > 0) {
             std::cout <<instruction << std::endl;
         }

         for (i = 0; i < n; i++) {
             const char *answer;
             const char *prompt;
             char echo;

             prompt = ssh_userauth_kbdint_getprompt(connection, i, &echo);
             if (prompt == NULL) {
                 break;
             }

             if (echo) {
                 char *p;

                 if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                     return SSH_AUTH_ERROR;
                 }

                 buffer[sizeof(buffer) - 1] = '\0';
                 if ((p = strchr(buffer, '\n'))) {
                     *p = '\0';
                 }

                 if (ssh_userauth_kbdint_setanswer(connection, i, buffer) < 0) {
                     return SSH_AUTH_ERROR;
                 }

                 memset(buffer, 0, strlen(buffer));
             } else {
                 if (user.getPassword().c_str() && strstr(prompt, "Password:")) {
                     answer = user.getPassword().c_str();
                 } else {
                     buffer[0] = '\0';

                     if (ssh_getpass(prompt, buffer, sizeof(buffer), 0, 0) < 0) {
                         return SSH_AUTH_ERROR;
                     }
                     answer = buffer;
                 }
                 err = ssh_userauth_kbdint_setanswer(connection, i, answer);
                 memset(buffer, 0, sizeof(buffer));
                 if (err < 0) {
                     return SSH_AUTH_ERROR;
                 }
             }
         }
         err=ssh_userauth_kbdint(connection,NULL,NULL);
     }

     return err;
}
SSHRemoteEndPoint *
SSHConnection::createEndPoint(Address destination, struct event_base * workerEventBase){
	ssh_channel forwarding_channel;

	forwarding_channel = ssh_channel_new(connection);

	ssh_set_log_level( SSH_LOG_FUNCTIONS );
	if (forwarding_channel == NULL) {
		return nullptr;
	}
	int rc = ssh_channel_open_forward(forwarding_channel,
									destination.getHost().c_str(), destination.getPort(),
									"localhost", 1);
	if (rc != SSH_OK)
	{
		ssh_channel_free(forwarding_channel);
		return nullptr;
	}

	num_channels++;
	num_active_channels++;
	SSHRemoteEndPoint * endPoint = new SSHRemoteEndPoint(host, user, destination, forwarding_channel, workerEventBase);
	return endPoint;
}
} /* namespace tunnels */
} /* namespace tunnelier */


