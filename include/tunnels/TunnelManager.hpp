/* TunnelManager.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef TUNNELMANAGER_HPP_
#define TUNNELMANAGER_HPP_
#include "libJPLogger.hpp"
#include "cxx11_implementations.hpp"
#include <map>
#include <vector>
#include <event.h>
#include <time.h>
namespace tunnelier {
class TunnelManager;
}
#include "tunnels/SocketListener.hpp"
#include "tunnels/SSHConnection.hpp"
#include "tunnels/Tunnel.hpp"
#include "tunnels/TunnelWorker.hpp"
#include "tunnels/LocalTunnelSSH.hpp"
#include "Address.hpp"
#include "User.hpp"
#include "libJPLogger.hpp"


namespace tunnelier {
typedef struct {
	TunnelManager * manager;
	tunnels::SocketListener * listener;
} Manager_SocketListener;
typedef struct{
	TunnelManager * manager;
	tunnels::LocalTunnelSSH *localSocket;
	tunnels::TunnelWorker * worker;
	struct event* event;
	tunnels::SSHConnection * sshConnection;
} ManagerTunnelWorker;
/**
 * Class that manage everything
 */
class TunnelManager {
public:
	/**
	 * Class constructor
	 * @param numberWorkers Number of threads to be created
	 */
	TunnelManager(int numberWorkers);
	/**
	 * TClass destructor
	 */
	virtual ~TunnelManager();
	/**
	 * Create one Tunnel
	 * @param localPort Port to listen
	 * @param middleAddress Middle address
	 * @param middleUser User in the middle machine
	 * @param destination Tunnel destination machine
	 */
	int createTunnel(int localPort, Address middleAddress, User middleUser, Address destination);
	/**
	 * Close a tunnel
	 * @param localPort Port to close
	 * @param forceClose Force to close all SSH connections or not
	 */
	int closeTunnel(int localPort, bool forceClose);
	/**
	 * Create the listener object
	 * @param localPort Port to listen
	 * @param destination Tunnel destination machine
	 */
	int createListener(int localPort, Address destination );
	/**
	 * Create the SSH connection
	 * @param host Host information
	 * @param user User information
	 */
	int createSSHConnection( Address host, User user);
	/**
	 * Check if there is an SSH connection opent to the host/user
	 * @param host Host information
	 * @param user User information
	 */
	int isSSHConnectionOpen( Address host, User user);
	/**
	 * Retrieve the next available worker
	 */
	tunnels::TunnelWorker * nextAvailableWorker();
	/**
	 * Link a port with a listener
	 * @param localPort
	 * @param listener
	 */
	void link(int localPort, tunnels::SocketListener* listener);

	/**
	 * List of call backs to be used by the libevent
	 */
	/**
	 * Poll the tunnels for information
	 * @param fd dummy
	 * @param event dummy
	 * @param arg Arguments to be passed to the function
	 */
	static void poolTunnels(int fd, short event, void *arg);
	/**
	 * Write statistic information
	 * @param fd dummy
	 * @param event dummy
	 * @param arg Arguments to be passed to the function
	 */
	static void stats(int fd, short event, void *arg);
	/**
	 * Accept a connection in the listener
	 * @param listener Listener that receive the connection
	 * @param fd Socket of the new connection
	 * @param address Address information of the connection
	 * @param socklen
	 * @param ctx Arguments to be passed to the function
	 */
	static void acceptFromListener_cb(struct evconnlistener *listener,
		    evutil_socket_t fd, struct sockaddr *address, int socklen,
		    void *ctx);
	/**
	 * Accept connection error call back
	 * @param listener Listener with error
	 * @param ctx Arguments to be passed to the function
	 */
	static void acceptError_cb(struct evconnlistener *listener, void *ctx);
	/**
	 * Close a socket
	 * @param socket_id Socket to be closed
	 * @param event dummy
	 * @param ctx Arguments to be passed to the function
	 */
	static void localSocketClose(int socket_id, short event, void * ctx);
	/**
	 * Clean up connections that should not be active
	 * @param socket_id dummy
	 * @param event dummy
	 * @param ctx Arguments to be passed to the function
	 */
	static void cleanUp(int socket_id, short event, void * ctx);
private:
	/**
	 * Unused tunnels
	 */
	std::vector<tunnels::LocalTunnelSSH* > freeTunnels;
	/**
	 * Tunnels currently being used
	 */
	std::vector<tunnels::LocalTunnelSSH*> activeTunnels;
	/**
	 * Open SSH Connection
	 */
#ifdef USE_BOOST_INSTEAD_CXX11
	std::map<std::pair<Address,User>,std::vector<tunnels::SSHConnection*> > openConnections;
#else
	std::map<std::tuple<Address,User>,std::vector<tunnels::SSHConnection*> > openConnections;
#endif
	/**
	 * Open listeners
	 */
	std::map<int, tunnels::SocketListener*> openListeners;
	/**
	 * Map between port number and destination
	 */
#ifdef USE_BOOST_INSTEAD_CXX11
	std::map<int,boost::tuple<Address,User,Address> > tunnelLink;
#else
	std::map<int,std::tuple<Address,User,Address> > tunnelLink;
#endif
	/**
	 * Workers
	 */
	std::vector<tunnels::TunnelWorker * > workers;
	/**
	 * Worker for management
	 */
	tunnels::TunnelWorker * managementWorker;
	/**
	 * Polling event
	 */
	event *poll_event;
	/**
	 * Statistics event
	 */
	event *stats_event;
	/**
	 * Clean up event
	 */
	event *cleanup_event;
	/**
	 * time variable
	 */
	struct timeval tv;
	std::mutex mutex;
	/**
	 * Log name
	 */
	static std::string LOGNAME;
};

} /* namespace tunnelier */
#endif /* TUNNELMANAGER_HPP_ */

