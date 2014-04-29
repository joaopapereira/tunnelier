/*
 * TunnelManager.cpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#include "tunnels/TunnelManager.hpp"
#include <algorithm>
#include <iostream>


namespace tunnelier {
using namespace tunnels;

TunnelManager::TunnelManager(int numberWorkers) {
	for( int i = 0 ; i < numberWorkers; i++ ){
		workers.push_back((new tunnels::TunnelWorker())->start());
	}
	tunnels::TunnelWorker * w = workers.at(0);
	std::cout << "Tunnels starting" << std::endl;
	tv.tv_sec = 1;
	sleep(1);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	std::cout << "Going to add to the base: " << w->getEventBase()  << std::endl;
	poll_event = evtimer_new(w->getEventBase(), TunnelManager::poolTunnels, (void*)this);
	w->addWork();
	//event_assign(poll_event, w->getEventBase(), -1, flags, timeout_cb, (void*) this);
	//evtimer_add(ssh_event, static_cast<const timeval*>(&tv));
	evtimer_add(poll_event, static_cast<const timeval*>(&tv));

}

TunnelManager::~TunnelManager() {
	// TODO Auto-generated destructor stub
	for(auto t: openListeners){
		delete std::get<1>(t);
	}
	event_del(poll_event);
	for(tunnels::TunnelWorker * w: workers){
		w->stop_join();
		delete w;
	}
	workers.clear();
	for(auto tunnel: activeTunnels){
		delete tunnel;
	}
	activeTunnels.clear();
	for(auto tunnel: freeTunnels){
		delete tunnel;
	}
	freeTunnels.clear();
}



int TunnelManager::createTunnel(int localPort, Address middleAddress,
		User middleUser, Address destination) {
	std::lock_guard<std::mutex> lock(mutex);
	std::cout << "Create Tunnel!!" << std::endl;
	if( 0 == createListener(localPort, destination) ){
		std::cout << "Check if connection is open" << std::endl;
		if( 0 == isSSHConnectionOpen( middleAddress, middleUser) )
			return 0;
		std::cout << "Creating a new SSH Connection!" << std::endl;
		if( 0 == createSSHConnection( middleAddress, middleUser) ){
			tunnelLink[localPort] = std::make_tuple(middleAddress, middleUser, destination);
			return 0;
		}
		SocketListener * listener = openListeners[localPort];
		openListeners.erase(localPort);
		delete listener;
		std::cout << "Error executing the connect" << std::endl;
		return -2;
	}
	std::cout << "Error creating Listener tunnel not created" << std::endl;
	return -1;
}

int TunnelManager::createListener(int localPort,
		Address destination) {
	std::cout << "Create Listener!!" << std::endl;
	try{
		openListeners.at(localPort);
		return -1;
	}catch(std::out_of_range &e){}
	TunnelWorker* worker = nextAvailableWorker();
	if( nullptr == worker )
		return -2;
	SocketListener * listener = new SocketListener( worker->getEventBase(), localPort);
	worker->addWork();
	if( nullptr == listener)
		return -3;
	listener->setAcceptCallBack(acceptFromListener_cb);
	listener->setErrorCallBack(acceptError_cb);
	Manager_SocketListener *aux = new Manager_SocketListener();
	aux->manager = this;
	aux->listener = listener;
	listener->bind(aux);
	openListeners[localPort] = listener;
	std::cout << "Listener created with success!" << std::endl;
	return 0;
}

int TunnelManager::createSSHConnection(Address host, User user) {
	std::cout << "Create SSHConnection!!" << std::endl;
	int result = isSSHConnectionOpen(host, user);
	if( result > 0){
		std::cout << "Is already open: " << result << std::endl;
		return 0;
	}
	openConnections.insert(std::make_pair<std::tuple<Address,User>,std::vector<tunnels::SSHConnection*> >(std::make_tuple(host,user),std::vector<tunnels::SSHConnection*>()));
	SSHConnection * connection = new SSHConnection(host, user);
	if( 0 == connection->connect() ){
		std::cout << "Connection created with success" << std::endl;
		openConnections[std::make_tuple(host,user)].push_back(connection);
		return 0;
	}
	std::cout << "Unable to create connection" << std::endl;
	return -1;
}

int TunnelManager::isSSHConnectionOpen(Address host, User user) {
	try{
		auto vec = openConnections.at(std::make_pair(host,user));
		return vec.size();
	}catch(std::out_of_range &e){}
	catch(...){}
	return -1;
}

TunnelWorker* TunnelManager::nextAvailableWorker() {
	sort(workers.begin(), workers.end(), [](TunnelWorker * a, TunnelWorker * b) {
		return b->getCurrentAmountOfWork() > a->getCurrentAmountOfWork();
	});
	return *workers.begin();
}


void TunnelManager::poolTunnels(int fd, short event, void* arg) {
	TunnelManager * manager = static_cast<TunnelManager*>(arg);
	std::lock_guard<std::mutex> lock(manager->mutex);
	//std::cout << "Going to poll!!!"<<std::endl;
	for(auto tunnel: manager->activeTunnels){
		if( 1 == tunnel->poll() ){
			TunnelWorker * w = manager->nextAvailableWorker();
			event_base_once(w->getEventBase(), 1, EV_READ, tunnel->channel_to_socket, tunnel, nullptr);
		}
			
	}
	evtimer_add(manager->poll_event, static_cast<const timeval*>(&(manager->tv)));
}

void  TunnelManager::acceptFromListener_cb(struct evconnlistener *listener,
		    evutil_socket_t fd, struct sockaddr *address, int socklen,
		    void *ctx){
	Manager_SocketListener * arg = static_cast<Manager_SocketListener*>(ctx);
	std::cout << "Received connection :D"<< std::endl;
	TunnelManager * manager = arg->manager;
	SocketListener * listen = arg->listener;
	auto res = manager->tunnelLink[listen->getLocalPort()];
	SSHRemoteEndPoint endpoint(std::get<0>(res), std::get<1>(res), std::get<2>(res));
	TunnelWorker * worker = manager->nextAvailableWorker();
	TunnelWorker * w1;
	//auto it = std::find(manager->freeTunnels.begin(), manager->freeTunnels.end(),&endpoint);
	tunnels::LocalTunnelSSH * sshTunnel = nullptr;
	auto delCursor = manager->freeTunnels.begin();
	for(auto it: manager->freeTunnels ){
		if(*static_cast<SSHRemoteEndPoint*>(it->getRemoteEndPoint()) == endpoint){
			sshTunnel = it;
			break;
		}
		delCursor++;
	}

	tunnels::LocalSocket *localSocket = new tunnels::LocalSocket(fd, worker->getEventBase());
	worker->addWork();
	//if( it == manager->freeTunnels.end()){
	if(sshTunnel == nullptr){
		std::cout <<"Creating new fw channel!"<<std::endl;
		tunnels::SSHRemoteEndPoint * sshEndPoint;
		tunnels::SSHConnection* tunnel = nullptr;
		try{
			auto allTunnel = manager->openConnections.at(std::make_tuple(std::get<0>(res), std::get<1>(res)));
			for( auto t1: allTunnel ){
				if( t1->canCreateChannel()){
					tunnel = t1;
					break;
				}
			}
			if( nullptr == tunnel )
				tunnel = new tunnels::SSHConnection(std::get<0>(res), std::get<1>(res));
			sshEndPoint = tunnel->createEndPoint(std::get<2>(res), worker->getEventBase());
		}catch(std::out_of_range &e){
			std::cout << "Connection not created!....." << e.what() << std::endl;
			delete localSocket;
			worker->removeWork();
			return;
		}catch( const std::ios_base::failure & e){
			std::cout << "Connection not created!....." << e.what() << std::endl;
			delete localSocket;
			worker->removeWork();
			return;
		}
		ManagerTunnelWorker * container = new ManagerTunnelWorker();
		container->sshConnection = tunnel;
		container->event = event_new(worker->getEventBase(), -1, EV_READ, localSocketClose, container);
		event_add(container->event, nullptr);
		container->manager = manager;
		container->worker = worker;
		sshTunnel = new tunnels::LocalTunnelSSH(localSocket, sshEndPoint, container->event);
		container->localSocket = sshTunnel;

	}else{
		std::cout <<"Using fw channel already open!"<<std::endl;
				sshTunnel->setLocalSocket(localSocket);
		manager->freeTunnels.erase(delCursor);
	}
	localSocket->setReadCallBack(sshTunnel->socket_to_ssh);
	localSocket->setErrorCallBack(sshTunnel->errorcb);
	localSocket->bindSocket(sshTunnel);

	manager->activeTunnels.push_back(sshTunnel);
}
void
TunnelManager::acceptError_cb(struct evconnlistener *listener, void *ctx){
	Manager_SocketListener * arg = static_cast<Manager_SocketListener*>(ctx);
	std::cout << "Error puff"<<std::endl;
}
void
TunnelManager::localSocketClose(int socket_id, short event, void * ctx){
	ManagerTunnelWorker * container = static_cast<ManagerTunnelWorker *>(ctx);
	std::lock_guard<std::mutex> lock(container->manager->mutex);
	std::cout << "Entered localSocketClose" << std::cout;
	auto it = find(container->manager->activeTunnels.begin(),
				   container->manager->activeTunnels.end(),
				   container->localSocket);
	if( it == container->manager->activeTunnels.end()){
		std::cout << "Something strange happened closing tunnel is not in the active lot...." << std::endl;
		//free(container->localSocket);
		delete container->localSocket;

	}else{
		container->manager->activeTunnels.erase(it);
		//container->manager->freeTunnels.push_back(container->localSocket);
		delete container->localSocket;
	}
	container->worker->removeWork();
	std::cout << "Ended localSocketClose" << std::cout;
}
} /* namespace tunnelier */
