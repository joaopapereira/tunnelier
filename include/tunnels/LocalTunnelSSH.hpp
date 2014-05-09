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



namespace tunnelier {
namespace tunnels {

class LocalTunnelSSH{
public:
	LocalTunnelSSH(LocalSocket * local, SSHRemoteEndPoint *remote, struct event *connectionEndEvent):
		local(local),
		remote(remote),
		managerBase(nullptr),
		channel_to_socket_event(nullptr),
		connectionEnd(connectionEndEvent){
		//remote->setReadCallback(this->copy_chan_to_fd, this);
		remote->setReadCallBack(this->channel_to_socket, this);
	};
	virtual ~LocalTunnelSSH(){
		std::cout << "Destructing a tunnel"<<std::endl << std::endl;
		std::lock_guard<std::recursive_mutex> lock(mutex);
		if( nullptr != channel_to_socket_event){
			event_del(channel_to_socket_event);
			event_free(channel_to_socket_event);
		}
		delete local;
		delete remote;
		std::cout << "Tunnel destroyed"<<std::endl << std::endl;
	};
	inline void setLocalSocket(LocalSocket * localSocket){
		std::cout << "Entered setLocalSocket"<<std::endl;
		std::lock_guard<std::recursive_mutex> lock(mutex);
		std::cout << "Passed mutex on setLocalSocket"<<std::endl;
		if( local != nullptr )
			delete local;
		local = localSocket;
		std::cout << "Ended setLocalSocket"<<std::endl;
	};
	inline bool operator==(SSHRemoteEndPoint &rhs){
		return *static_cast<SSHRemoteEndPoint*>(remote)== rhs;
	};
#if USE_UNIX_SOCKET
	static void socket_to_ssh(int socket_id, short event, void * ctx){
		std::cout << "Entered socket_to_ssh"<<std::endl;
		LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(ctx);
		std::lock_guard<std::recursive_mutex> lock(con->mutex);
		std::cout << "Passed mutex on socket_to_ssh"<<std::endl;
		char data[4096];
		int length;
		while( (length = read(socket_id, data, sizeof(data) ) ) > 0 ){
			data[length] = 0;
			con->remote->writeToEndPoint(data, length);
			std::cout << "packet data: "<< data<<std::endl;
			std::cout << "socket_to_ssh" << " Packet sent!"<<std::endl;
		}
		std::cout << "Exit socket_to_ssh"<<std::endl;
	};
#else
	static void socket_to_ssh(struct bufferevent *bev, void *ctx){
		std::cout << "Entered socket_to_ssh"<<std::endl;
		LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(ctx);
		std::lock_guard<std::recursive_mutex> lock(con->mutex);
		std::cout << "Passed mutex on socket_to_ssh"<<std::endl;
		struct evbuffer *input = bufferevent_get_input(bev);
		char data[4096];
		int length;

		std::cout << "The buffer as: " << evbuffer_get_length(input) << std::endl;
		while ((length = evbuffer_remove(input, data, sizeof(data))) > 0) {
			std::cout << "Retrieve " << length << " from socket" << std::endl;
			data[length] = 0;
			con->remote->writeToEndPoint(data, length);
			std::cout << "packet data: "<< data<<std::endl;
			std::cout << "socket_to_ssh" << " Packet sent!"<<std::endl;
		}
		//while( con->local->moveDataTo(con->remote) > 0);
		std::cout << "Exit socket_to_ssh"<<std::endl;
	};
#endif
	static void errorcb(struct bufferevent *bev, short events, void *ctx){
		LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(ctx);
		std::cout << "Entered errorcb!!"<<std::endl;
		std::lock_guard<std::recursive_mutex> lock(con->mutex);
		std::cout << "Passed mutex on errorcb"<<std::endl;
		int finished = 0;
		if (events & BEV_EVENT_EOF) {
			/* connection has been closed, do any clean up here */
			std::cout << "Closing connection EOF with:" << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())<<std::endl;
			//socket_to_ssh(bev, ctx);
			finished = 1;

		} else if (events & BEV_EVENT_ERROR) {
			/* check errno to see what error occurred */
			std::cout << "Closing connection with:" << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())<<std::endl;
			finished = 1;
		} else if (events & BEV_EVENT_TIMEOUT) {
			/* must be a timeout event handle, handle it */
			/* ... */
		}

		if (finished) {
			bufferevent_free(bev);
			// TODO: Missing code to move the tunnel back to free
			event_active(con->connectionEnd, EV_READ, 0);
		}

		std::cout << "Ended errorcb"<<std::endl;
	};
	static int copy_chan_to_fd(ssh_session session,
	                                           ssh_channel channel,
	                                           void *data,
	                                           uint32_t len,
	                                           int is_stderr,
	                                           void *userdata){
		std::cout << "Entered copy_chan_to_fd"<<std::endl;
		LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(userdata);
		std::lock_guard<std::recursive_mutex> lock(con->mutex);
		std::cout << "Passed mutex on copy_chan_to_fd"<<std::endl;
		//con->remote->setData(data, len);
		//return len - con->remote->directCopy(con->local, data, len);
		std::cout << "packet data: "<< static_cast<char*>(data)<<std::endl;
		con->poll();
		//int processed = con->local->writeToEndPoint(data, len);
		//std::cout << "Exit copy_chan_to_fd processing:" << processed << " of "<< len<<std::endl;
		std::cout << "Exit copy_chan_to_fd processing"<< std::endl;
		return 0;
	};
	static void channel_to_socket(int socket_id, short event, void * ctx){
		std::cout << "Entered channel_to_socket"<<std::endl;
		LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(ctx);
		std::lock_guard<std::recursive_mutex> lock(con->mutex);
		std::cout << "Passed mutex on channel_to_socket"<<std::endl;
		con->remote->writeToSocket( con->local->getSocketId());
		std::cout << "Exit socket_to_ssh"<<std::endl;
	};
	int poll(){
		int result = 0;
		if( 1 == remote->poll() ){
#if !USE_UNIX_SOCKET
			local->getWorkerEventBase();
			struct event *ev = event_new(local->getWorkerEventBase(), -1, EV_READ, this->channel_to_socket, this);
			event_add(ev, nullptr);
			event_active(ev, EV_READ, 0);
#else
			channel_to_socket(-1,-1, static_cast<void*>(this));
#endif
			result = 1;
		}
		local->poll();
		return result;
	};
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
};
} /* namespace tunnels */
} /* namespace tunnelier */


#endif /* LOCALTUNNELSSH_HPP_ */
