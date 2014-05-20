/*
 * SSHRemoteEndPoint.cpp
 *
 *  Created on: Apr 16, 2014
 *      Author: joao
 */

#include <tunnels/SSHRemoteEndPoint.hpp>
#include <iostream>

namespace tunnelier {
namespace tunnels {
using namespace jpCppLibs;
SSHRemoteEndPoint::SSHRemoteEndPoint(Address middleHost, User middleUser, Address destination):
	EndPoint(),
	middleHost(middleHost),
	middleUser(middleUser),
	address(destination),
	channel(nullptr),
	socket_event(nullptr),
	readCallBack(nullptr),
	workerEventBase(nullptr),
	LOGNAME("REP"){

}
SSHRemoteEndPoint::SSHRemoteEndPoint(Address middleHost, User middleUser, Address destination, ssh_channel channel, struct event_base * workerEventBase ):
		EndPoint(),
		middleHost(middleHost),
		middleUser(middleUser),
		address(destination),
		channel(channel),
		socket_event(event_new(workerEventBase, 1, EV_READ, nullptr, nullptr)),
		readCallBack(nullptr),
		workerEventBase(workerEventBase),
		LOGNAME("REP"){
}
SSHRemoteEndPoint::~SSHRemoteEndPoint() {
	if(nullptr != channel){
		if(channel && ssh_channel_is_open(channel) )
			ssh_channel_close(channel);
		ssh_channel_free(channel);
	}
	if(nullptr != socket_event)
		event_free(socket_event);
}

int SSHRemoteEndPoint::poll() {
	std::lock_guard<std::mutex> lock(mutex);
	int result = channel && \
	ssh_channel_is_open(channel) &&
	ssh_channel_poll(channel,0);
	if( result > 0 ){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
			<< "Scheduling read!" << std::endl;

		event_add(socket_event, nullptr);
	}
	return result;
}
int SSHRemoteEndPoint::writeToEndPoint( void* data, int length){
	std::lock_guard<std::mutex> lock(mutex);
	return ssh_channel_write(channel, data, length);
}
int SSHRemoteEndPoint::readFromEndPoint(){
	std::lock_guard<std::mutex> lock(mutex);
	return ssh_channel_read(channel, data, length,0);
}
int SSHRemoteEndPoint::writeToSocket(int socket_fd){
	std::lock_guard<std::mutex> lock(mutex);
	int r;
	char sendBuff[2048];
	int lus;
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
				<< "Entered writeToSocket" << std::endl;
	while(channel && ssh_channel_is_open(channel) && (r = ssh_channel_poll(channel,0))!=0){
                lus=ssh_channel_read(channel,sendBuff,sizeof(sendBuff) > r ? r : sizeof(sendBuff),0);
                if(lus==-1){
                	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_ERR)
                					<< "Error reading from channel!!" << std::endl;
                    return -1;
                }
                if(lus==0){
                    ssh_channel_free(channel);
                    channel=NULL;
                } else
                	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG)
							<< "Going to write to socket" << std::endl;
                  if (write(socket_fd, sendBuff,lus) < 0) {
                	  OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_ERR)
							<< "Error writting to socket!!" << std::endl;
                    return -1;
              }
        }
	
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC)
					<< "Exit writeToSocket" << std::endl;
}
} /* namespace tunnels */
} /* namespace tunnelier */
