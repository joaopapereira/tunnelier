/*
 * LocalTunnelSSH.hpp
 *
 *  Created on: Apr 10, 2014
 *      Author: joao
 */

#ifndef LOCALTUNNELSSH_HPP_
#define LOCALTUNNELSSH_HPP_

#include <mutex>
#include "tunnels/LocalSocket.hpp"
#include "tunnels/Tunnel.hpp"
#include "tunnels/SSHRemoteEndPoint.hpp"
#include "libJPLogger.hpp"


namespace tunnelier {
namespace tunnels {

class LocalTunnelSSH{
public:
	LocalTunnelSSH(LocalSocket * local, SSHRemoteEndPoint *remote, struct event *connectionEndEvent);
	virtual ~LocalTunnelSSH();
	void setLocalSocket(LocalSocket * localSocket);
	inline bool operator==(SSHRemoteEndPoint &rhs){
		return *static_cast<SSHRemoteEndPoint*>(remote)== rhs;
	};
#if USE_UNIX_SOCKET
	static void socket_to_ssh(int socket_id, short event, void * ctx);
#else
	static void socket_to_ssh(struct bufferevent *bev, void *ctx);
#endif
	static void errorcb(struct bufferevent *bev, short events, void *ctx);
	static int copy_chan_to_fd(ssh_session session,
	                                           ssh_channel channel,
	                                           void *data,
	                                           uint32_t len,
	                                           int is_stderr,
	                                           void *userdata);
	static void channel_to_socket(int socket_id, short event, void * ctx);
	int poll();
	inline LocalSocket * getLocalEndPoint(){
		return local;
	};
	inline SSHRemoteEndPoint * getRemoteEndPoint(){
		return remote;
	};
private:
	struct event_base *managerBase;
	std::recursive_mutex mutex;
	std::mutex simple_mutex;
	LocalSocket *local;
	SSHRemoteEndPoint *remote;
	struct event *channel_to_socket_event;
	struct event *connectionEnd;
	std::string LOGNAME;
};

} /* namespace tunnels */
} /* namespace tunnelier */


#endif /* LOCALTUNNELSSH_HPP_ */
