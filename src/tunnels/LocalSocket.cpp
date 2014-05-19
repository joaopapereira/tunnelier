/*
 * LocalSocket.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: joao
 */

#include "tunnels/LocalSocket.hpp"
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <event.h>
#include <sys/socket.h>
using namespace tunnelier::tunnels;
using namespace jpCppLibs;
LocalSocket::LocalSocket(int socket_fd, struct event_base * workerEventBase):
		EndPoint(),

		//socket_event(event_new(workerEventBase, socket_fd, EV_READ, nullptr, nullptr)),
		readCallBack(nullptr),
		errorCallBack(nullptr),
		socket_id(socket_fd),
		workerEventBase(workerEventBase),
		LOGNAME("LSOC"){
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
				<< "Creating LocalSocket for fd: '"
				<< socket_fd << "' the worker is: " << workerEventBase << std::endl;

#if !USE_UNIX_SOCKET
	/*socket_event = bufferevent_socket_new(
			workerEventBase, socket_fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_UNLOCK_CALLBACKS );*/
#else
	socket_event = event_new(workerEventBase, socket_fd, EV_READ, nullptr, nullptr);
#endif

}
LocalSocket::~LocalSocket(){
#if !USE_UNIX_SOCKET
	/*if( nullptr != socket_event)
		bufferevent_free(socket_event);*/
#else
	close(socket_id);
	event_free(socket_event);
#endif

}
#if !USE_UNIX_SOCKET
int
LocalSocket::writeToEndPoint(void* data, int length){
	//UserConnection * con = static_cast<UserConnection *>(ctx);
	//bufferevent_get_base()
	/*struct evbuffer *input = bufferevent_get_input(bev);
	while ((length = evbuffer_remove(input, data, sizeof(data))) > 0) {
		data[length] = 0;
		con->sshChannel->writeToSSH(data, length);
	}*/
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
					<< "Entered writeToEndPoint" << std::endl;
	if( 0 == bufferevent_write(socket_event, data,length) )
		return length;

	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
						<< "Exit writeToEndPoint" << std::endl;
}
#else
int
LocalSocket::writeToEndPoint(void* data, int length){
	if( write(socket_id, data,length) < 0 ){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_ERR)
								<< Error  sending to socket:" << errno << std::endl;
		return 0;
	}
	return length;
}
#endif
void
LocalSocket::bindSocket( void * arguments){
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
						<< "Entered bindSocket" << std::endl;
#if !USE_UNIX_SOCKET
	socket_event = bufferevent_socket_new(
				workerEventBase, socket_id, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_UNLOCK_CALLBACKS );
	bufferevent_setcb(socket_event, readCallBack, NULL, errorCallBack, arguments);
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
							<< "Enable event" << std::endl;
	bufferevent_enable(socket_event, EV_READ|EV_WRITE);
#else
	event_assign(socket_event, workerEventBase, socket_id, EV_READ, readCallBack, arguments);
#endif
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
								<< "Exit bindSocket" << std::endl;
}

int
LocalSocket::poll(){
#if USE_UNIX_SOCKET
	fd_set read_flags,write_flags;
	int res;
	FD_ZERO(&read_flags);
	FD_ZERO(&write_flags);
	FD_SET(socket_id, &read_flags);
	struct timeval timeout;
	timeout.tv_sec=1;
        timeout.tv_usec=0;
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
									<< "Pooling local socket" << std::endl;
	res = select( socket_id+1, &read_flags,NULL,NULL,&timeout);
	if( res < 0 )
	    return -1;
	else if( FD_ISSET(socket_id , &read_flags )){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG)
											<< "Socket have information" << std::endl;
		event_add(socket_event, nullptr);
	}
	return -2;
#endif
}
