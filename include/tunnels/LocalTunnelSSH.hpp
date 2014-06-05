/* LocalSocket.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef LOCALTUNNELSSH_HPP_
#define LOCALTUNNELSSH_HPP_

#include "libJPLogger.hpp"
#include "cxx11_implementations.hpp"
#include "tunnels/LocalSocket.hpp"
#include "tunnels/Tunnel.hpp"
#include "tunnels/SSHRemoteEndPoint.hpp"


namespace tunnelier {
namespace tunnels {
/**
 * Class that model the Tunnel
 */
class LocalTunnelSSH{
public:
	/**
	 * Class constructor
	 * @param local Local socket
	 * @param remote SSH Connection forwarding port
	 * @param connectionEndEvent Event that informs that the connection should be closed
	 */
	LocalTunnelSSH(LocalSocket * local, SSHRemoteEndPoint *remote, struct event *connectionEndEvent);
	/**
	 * Class destructor
	 */
	virtual ~LocalTunnelSSH();
	/**
	 * Set the local socket
	 * @param localSocket New local socket
	 */
	void setLocalSocket(LocalSocket * localSocket);
	/**
	 * Overload == operator
	 * @param rhs Tunnel to compare to
	 */
	inline bool operator==(SSHRemoteEndPoint &rhs){
		return *static_cast<SSHRemoteEndPoint*>(remote)== rhs;
	};
#if USE_UNIX_SOCKET
	static void socket_to_ssh(int socket_id, short event, void * ctx);
#else
	/**
	 * Copy information from the local socket into the SSH tunnel
	 * Function called by libevent
	 * @param bev Event buffer with socket data
	 * @param ctx Context variable
	 */
	static void socket_to_ssh(struct bufferevent *bev, void *ctx);
#endif
	/**
	 * Error callback
	 * Function called by libevent
	 * @param bev Event buffer with socket data
	 * @param events Type of error
	 * @param ctx Context variable
	 */
	static void errorcb(struct bufferevent *bev, short events, void *ctx);
	/**
	 * Copy from SSH Channeld to local socket
	 * Used as callback to libssh
	 * @deprecated
	 */
	static int copy_chan_to_fd(ssh_session session,
	                                           ssh_channel channel,
	                                           void *data,
	                                           uint32_t len,
	                                           int is_stderr,
	                                           void *userdata);
	/**
	 * Copy from SSH Channel to local socket
	 * Called by libevent
	 * @param socket_id Socket identifier
	 * @param event Type of event
	 * @param ctx Context variable
	 */
	static void channel_to_socket(int socket_id, short event, void * ctx);
	/**
	 * Poll the Tunnel to see if any of the sockets
	 * have data to be transfered
	 * @return 0 If there is something to be moved
	 */
	int poll();
	/**
	 * Retrieve Local End Point
	 */
	inline LocalSocket * getLocalEndPoint(){
		return local;
	};
	/**
	 * Retrieve the Remove End Point
	 */
	inline SSHRemoteEndPoint * getRemoteEndPoint(){
		return remote;
	};
private:
	/**
	 * Event base
	 */
	struct event_base *managerBase;
	/**
	 * Mutex to ensure only 1 thread at the time
	 * try to move information around
	 */
	std::recursive_mutex mutex;
	/**
	 *
	 */
	std::mutex simple_mutex;
	/**
	 * Local end point
	 */
	LocalSocket *local;
	/**
	 * Remote end point
	 */
	SSHRemoteEndPoint *remote;
	/**
	 * Event for moving data from remote to local
	 */
	struct event *channel_to_socket_event;
	/**
	 * Event of connection end
	 */
	struct event *connectionEnd;
	/**
	 * Log name
	 */
	std::string LOGNAME;
};

} /* namespace tunnels */
} /* namespace tunnelier */


#endif /* LOCALTUNNELSSH_HPP_ */
