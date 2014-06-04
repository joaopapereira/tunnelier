/*
 * TunnelManager.hpp
 *
 *  Created on: Apr 4, 2014
 *      Author: joao
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
class TunnelManager {
public:
	/**
	 *
	 */
	TunnelManager(int numberWorkers);
	/**
	 *
	 */
	virtual ~TunnelManager();
	/**
	 *
	 */
	int createTunnel(int localPort, Address middleAddress, User middleUser, Address destination);
	int closeTunnel(int localPort, bool forceClose);
	int createListener(int localPort, Address destination );
	int createSSHConnection( Address host, User user);
	int isSSHConnectionOpen( Address host, User user);
	tunnels::TunnelWorker * nextAvailableWorker();
	void link(int localPort, tunnels::SocketListener* listener);

	static void poolTunnels(int fd, short event, void *arg);
	static void stats(int fd, short event, void *arg);
	static void acceptFromListener_cb(struct evconnlistener *listener,
		    evutil_socket_t fd, struct sockaddr *address, int socklen,
		    void *ctx);
	static void acceptError_cb(struct evconnlistener *listener, void *ctx);
	static void localSocketClose(int socket_id, short event, void * ctx);
	static void cleanUp(int socket_id, short event, void * ctx);
private:

	std::vector<tunnels::LocalTunnelSSH* > freeTunnels;
	std::vector<tunnels::LocalTunnelSSH*> activeTunnels;
#ifdef USE_BOOST_INSTEAD_CXX11
	std::map<std::pair<Address,User>,std::vector<tunnels::SSHConnection*> > openConnections;
#else
	std::map<std::tuple<Address,User>,std::vector<tunnels::SSHConnection*> > openConnections;
#endif
	std::map<int, tunnels::SocketListener*> openListeners;
#ifdef USE_BOOST_INSTEAD_CXX11
	std::map<int,boost::tuple<Address,User,Address> > tunnelLink;
#else
	std::map<int,std::tuple<Address,User,Address> > tunnelLink;
#endif
	std::vector<tunnels::TunnelWorker * > workers;
	tunnels::TunnelWorker * managementWorker;
	event *poll_event;
	event *stats_event;
	event *cleanup_event;
	struct timeval tv;
	std::mutex mutex;
	static std::string LOGNAME;
};

} /* namespace tunnelier */
#endif /* TUNNELMANAGER_HPP_ */

