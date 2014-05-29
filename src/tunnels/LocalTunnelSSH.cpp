/*
 * LocalTunnelSSH.cpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#include "tunnels/LocalTunnelSSH.hpp"
namespace tunnelier {
namespace tunnels {
using namespace jpCppLibs;
LocalTunnelSSH::LocalTunnelSSH(LocalSocket * local, SSHRemoteEndPoint *remote, struct event *connectionEndEvent):
	local(local),
	remote(remote),
	managerBase(nullptr),
	channel_to_socket_event(nullptr),
	connectionEnd(connectionEndEvent),
	LOGNAME("TUN"){

	//remote->setReadCallback(this->copy_chan_to_fd, this);
	remote->setReadCallBack(this->channel_to_socket, this);
};
LocalTunnelSSH::~LocalTunnelSSH(){
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
			<< "Destructing a tunnel" << std::endl;
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::recursive_mutex*> lock(&mutex);
#else
	std::lock_guard<std::recursive_mutex> lock(mutex);
#endif
	if( nullptr != channel_to_socket_event){
		event_del(channel_to_socket_event);
		event_free(channel_to_socket_event);
	}
	delete local;
	delete remote;
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG)
				<< "Tunnel destroyed" << std::endl;
};
void
LocalTunnelSSH::setLocalSocket(LocalSocket * localSocket){
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
				<< "Entered setLocalSocket()" << std::endl;

#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::recursive_mutex*> lock(&mutex);
#else
	std::lock_guard<std::recursive_mutex> lock(mutex);
#endif
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_TRC)
					<< "setLocalSocket() passed lock" << std::endl;
	if( local != nullptr )
		delete local;
	local = localSocket;
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
					<< "Exit setLocalSocket()" << std::endl;
};

#if USE_UNIX_SOCKET
void
LocalTunnelSSH::socket_to_ssh(int socket_id, short event, void * ctx){

	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
					<< "Entered socket_to_ssh" << std::endl;
	LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(ctx);
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::recursive_mutex*> lock(&(con->mutex));
#else
	std::lock_guard<std::recursive_mutex> lock(con->mutex);
#endif

	OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_TRC)
						<< "Passed mutex socket_to_ssh" << std::endl;
	char data[4096];
	int length;
	while( (length = read(socket_id, data, sizeof(data) ) ) > 0 ){
		data[length] = 0;
		con->remote->writeToEndPoint(data, length);
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_TRC)
								<< "Packet data:" << data << std::endl;
	}

	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
						<< "Exit socket_to_ssh" << std::endl;
};
#else
void
LocalTunnelSSH::socket_to_ssh(struct bufferevent *bev, void *ctx){
	LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(ctx);
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
				<< "Entered socket_to_ssh()" << std::endl;

#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::recursive_mutex*> lock(&(con->mutex));
#else
	std::lock_guard<std::recursive_mutex> lock(con->mutex);
#endif
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_LOW, M_LOG_TRC)
					<< "socket_to_ssh() passed the lock" << std::endl;
	struct evbuffer *input = bufferevent_get_input(bev);
	char data[4096];
	int length;

	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_DBG)
			<< "The buffer size is: " << evbuffer_get_length(input) << std::endl;
	while ((length = evbuffer_remove(input, data, sizeof(data))) > 0) {
		OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_LOW, M_LOG_DBG)
					<< "Retrieve: " << length << " from socket" << std::endl;
		data[length] = 0;
		con->remote->writeToEndPoint(data, length);
	}
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
					<< "Exit socket_to_ssh()" << std::endl;
};
#endif
void
LocalTunnelSSH::errorcb(struct bufferevent *bev, short events, void *ctx){
	LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(ctx);
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
					<< "Entered errorcb()" << std::endl;
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::recursive_mutex*> lock(&(con->mutex));
#else
	std::lock_guard<std::recursive_mutex> lock(con->mutex);
#endif
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_LOW, M_LOG_TRC)
					<< "socket_to_ssh() passed mutex" << std::endl;
	int finished = 0;
	if (events & BEV_EVENT_EOF) {
		/* connection has been closed, do any clean up here */
		OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
						<< "Closing connection EOF with:"
						<< evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())
						<< std::endl;
		//socket_to_ssh(bev, ctx);
		finished = 1;

	} else if (events & BEV_EVENT_ERROR) {
		/* check errno to see what error occurred */
		OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
								<< "Closing connection with:"
								<< evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())
								<< std::endl;
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

	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
					<< "Exit socket_to_ssh()" << std::endl;
};
int
LocalTunnelSSH::copy_chan_to_fd(ssh_session session,
										   ssh_channel channel,
										   void *data,
										   uint32_t len,
										   int is_stderr,
										   void *userdata){
	LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(userdata);
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
						<< "Entered copy_chan_to_fd()" << std::endl;

#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::recursive_mutex*> lock(&(con->mutex));
#else
	std::lock_guard<std::recursive_mutex> lock(con->mutex);
#endif
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_LOW, M_LOG_TRC)
				<< "copy_chan_to_fd() passed mutex" << std::endl;
	//con->remote->setData(data, len);
	//return len - con->remote->directCopy(con->local, data, len);
	con->poll();
	//int processed = con->local->writeToEndPoint(data, len);
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
				<< "Exit copy_chan_to_fd()" << std::endl;
	return 0;
};
void
LocalTunnelSSH::channel_to_socket(int socket_id, short event, void * ctx){
	LocalTunnelSSH * con = static_cast<LocalTunnelSSH *>(ctx);
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
		<< "Entered channel_to_socket()" << std::endl;

#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::recursive_mutex*> lock(&(con->mutex));
#else
	std::lock_guard<std::recursive_mutex> lock(con->mutex);
#endif
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
			<< "channel_to_socket() passed mutex" << std::endl;
	con->remote->writeToSocket( con->local->getSocketId());
	OneInstanceLogger::instance().log(con->LOGNAME,M_LOG_NRM, M_LOG_TRC)
			<< "Exit channel_to_socket()" << std::endl;
};
int
LocalTunnelSSH::poll(){
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
};
};
