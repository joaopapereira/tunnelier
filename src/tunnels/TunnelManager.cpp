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
std::string TunnelManager::LOGNAME("MAN");
using namespace tunnels;
using namespace jpCppLibs;

TunnelManager::TunnelManager(int numberWorkers) {
	for( int i = 0 ; i < numberWorkers; i++ ){
		workers.push_back((new tunnels::TunnelWorker())->start());
	}
	tunnels::TunnelWorker * w = workers.at(0);
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC,"TunnelManager starting...");

	tv.tv_sec = 1;
	sleep(1);
	tv.tv_sec = 0;
	tv.tv_usec = 100;
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG,"Polling in the base %p",w->getEventBase() );

	poll_event = evtimer_new(w->getEventBase(), TunnelManager::poolTunnels, (void*)this);
	w->addWork();
	//event_assign(poll_event, w->getEventBase(), -1, flags, timeout_cb, (void*) this);
	//evtimer_add(ssh_event, static_cast<const timeval*>(&tv));
	w = this->nextAvailableWorker();
	evtimer_add(poll_event, static_cast<const timeval*>(&tv));
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG,"Statistics in the base %p",w->getEventBase() );
	stats_event = evtimer_new(w->getEventBase(), TunnelManager::stats, (void*)this);
	w->addWork();
	struct timeval tv1;
	tv1.tv_sec = 30;
	tv1.tv_usec = 0;
	evtimer_add(stats_event, static_cast<const timeval*>(&tv1));
	w = this->nextAvailableWorker();
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG,"Cleanup in the base %p",w->getEventBase() );
	cleanup_event = evtimer_new(w->getEventBase(), TunnelManager::cleanUp, (void*)this);
	w->addWork();
	tv1.tv_sec = 30;
	tv1.tv_usec = 0;
	evtimer_add(cleanup_event, static_cast<const timeval*>(&tv1));

}

TunnelManager::~TunnelManager() {
	std::lock_guard<std::mutex> lock(mutex);
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC,"Destroying tunnel manager" );
	// TODO Auto-generated destructor stub
	event_del(poll_event);
	event_del(stats_event);
	event_del(cleanup_event);
	for(auto t: openListeners){
		delete std::get<1>(t);
	}


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

	event_free(poll_event);
	event_free(stats_event);
	event_free(cleanup_event);
}



int TunnelManager::createTunnel(int localPort, Address middleAddress,
		User middleUser, Address destination) {
	std::lock_guard<std::mutex> lock(mutex);
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC,"Starting tunnel creation!");
	if( 0 == createListener(localPort, destination) ){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG,"Check if connection already open");
		if( 0 == isSSHConnectionOpen( middleAddress, middleUser) ){
			OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF,"Tunnel already exists!");
			return 0;
		}
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG,"Create SSH Connection!");
		if( 0 == createSSHConnection( middleAddress, middleUser) ){
			tunnelLink[localPort] = std::make_tuple(middleAddress, middleUser, destination);
			OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF,"Tunnel created successfully!");
			return 0;
		}
		SocketListener * listener = openListeners[localPort];
		openListeners.erase(localPort);
		delete listener;

		OneInstanceLogger::instance().log(LOGNAME,M_LOG_HGH, M_LOG_ERR,"Error connecting to Middle!");
		return -2;
	}

	OneInstanceLogger::instance().log(LOGNAME,M_LOG_HGH, M_LOG_ERR,"Error creating local listener!");

	return -1;
}
int
TunnelManager::closeTunnel(int localPort,bool forceClose) {
	std::lock_guard<std::mutex> lock(mutex);
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF,"Closing tunnel at: %d!", localPort);

	try{
		auto listener = openListeners.at(localPort);
		openListeners.erase(localPort);
		delete listener;
	}catch(...){ return -1;};
	try{
		auto tunnelInfo = tunnelLink[localPort];
		tunnelLink.erase(localPort);
		if( forceClose ){

		}
	}catch(...){ return -2;};
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF,"Tunnel closed");
	return 0;
}
int TunnelManager::createListener(int localPort,
		Address destination) {

	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF) <<
			"Creating local listener at port " << localPort <<
			" for address: " << destination << std::endl;
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
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF)
					<< "Listener created with success!" << std::endl;
	return 0;
}

int TunnelManager::createSSHConnection(Address host, User user) {
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG) <<
				"Creating SSH Connection against" << host <<
				" for user: " << user << std::endl;
	int result = isSSHConnectionOpen(host, user);
	if( result > 0){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
						"Connection already open!" << std::endl;
		return 0;
	}
	openConnections.insert(std::make_pair<std::tuple<Address,User>,std::vector<tunnels::SSHConnection*> >(std::make_tuple(host,user),std::vector<tunnels::SSHConnection*>()));
	SSHConnection * connection = new SSHConnection(host, user);
	if( 0 == connection->connect() ){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
								"Connection created with success" << std::endl;
		openConnections[std::make_tuple(host,user)].push_back(connection);
		TunnelWorker * worker = this->nextAvailableWorker();
		connection->setPollCallBack(TunnelManager::poolTunnels, this, worker->getEventBase());
		worker->addWork();
		return 0;
	}
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_WRN) <<
									"Unable to create connection" <<
									host << std::endl;
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
	std::cout << "Going to poll!!!"<<std::endl;
	for(auto tunnel: manager->activeTunnels){
		if( 1 == tunnel->poll() ){
			TunnelWorker * w = manager->nextAvailableWorker();
			event_base_once(w->getEventBase(), 1, EV_READ, tunnel->channel_to_socket, tunnel, nullptr);
		}
			
	}
	if(manager->activeTunnels.size() > 0){
		manager->tv.tv_sec = 0;
		manager->tv.tv_usec = 50;
	}else{
		manager->tv.tv_sec = 1;
		manager->tv.tv_usec = 0;
	}
	//evtimer_add(manager->poll_event, static_cast<const timeval*>(&(manager->tv)));
}

void  TunnelManager::acceptFromListener_cb(struct evconnlistener *listener,
		    evutil_socket_t fd, struct sockaddr *address, int socklen,
		    void *ctx){
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC) <<
									"New connection arrived!" << std::endl;
	Manager_SocketListener * arg = static_cast<Manager_SocketListener*>(ctx);
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
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG) <<
											"Creating new forwarding channel!" << std::endl;
		tunnels::SSHRemoteEndPoint * sshEndPoint;
		tunnels::SSHConnection* conn = nullptr;
		try{
			auto allConnections = manager->openConnections.at(std::make_tuple(std::get<0>(res), std::get<1>(res)));
			for( auto t1: allConnections){
				OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG) <<
							"Connection is open check if channel can be created!" << std::endl;
				if( t1->canCreateChannel()){
					OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG) <<
						"Channel can be created!" << std::endl;
					conn = t1;
					break;
				}
			}
			if( nullptr == conn ){
				OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG) <<
										"New connection need to be established" << std::endl;
				conn = new tunnels::SSHConnection(std::get<0>(res), std::get<1>(res));
				if( 0 == conn->connect() ){
					OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG) <<
											"Connection created with success" << std::endl;
					OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
									"Connection created with success" << std::endl;
					conn->setPollCallBack(TunnelManager::poolTunnels, manager, worker->getEventBase());
					worker->addWork();
					manager->openConnections[std::make_tuple(std::get<0>(res), std::get<1>(res))].push_back(conn);
				}else
					throw std::ios_base::failure("Unable to create new connection");
			}
			OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
									"Going to create End Point" << std::endl;
			sshEndPoint = conn->createEndPoint(std::get<2>(res), worker->getEventBase());
			if( nullptr == sshEndPoint ){
				throw std::ios_base::failure("Error creating SSH Channel!!!");
			}
		}catch(std::out_of_range &e){
			OneInstanceLogger::instance().log(LOGNAME,M_LOG_HGH, M_LOG_WRN) <<
												"Error creating connection:" << e.what() << std::endl;
			delete localSocket;
			worker->removeWork();
			return;
		}catch( const std::ios_base::failure & e){
			OneInstanceLogger::instance().log(LOGNAME,M_LOG_HGH, M_LOG_WRN) <<
						"Error creating connection points:" << e.what() << std::endl;

			delete localSocket;
			worker->removeWork();
			return;
		}
		ManagerTunnelWorker * container = new ManagerTunnelWorker();
		container->sshConnection = conn;
		container->event = event_new(worker->getEventBase(), -1, EV_READ, localSocketClose, container);
		event_add(container->event, nullptr);
		container->manager = manager;
		container->worker = worker;
		sshTunnel = new tunnels::LocalTunnelSSH(localSocket, sshEndPoint, container->event);
		container->localSocket = sshTunnel;

	}else{
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG) <<
				"Using fw channel already open" << std::endl;
		sshTunnel->setLocalSocket(localSocket);
		manager->freeTunnels.erase(delCursor);
	}
	localSocket->setReadCallBack(sshTunnel->socket_to_ssh);
	localSocket->setErrorCallBack(sshTunnel->errorcb);
	localSocket->bindSocket(sshTunnel);

	manager->activeTunnels.push_back(sshTunnel);

	event_base_once(event_get_base(manager->poll_event), 1, EV_READ, TunnelManager::poolTunnels, manager, nullptr);
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF) <<
					"New connection accepted" << std::endl;
}
void
TunnelManager::acceptError_cb(struct evconnlistener *listener, void *ctx){
	Manager_SocketListener * arg = static_cast<Manager_SocketListener*>(ctx);
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_WRN) <<
						"Error accepting call" << std::endl;
}
void
TunnelManager::localSocketClose(int socket_id, short event, void * ctx){
	ManagerTunnelWorker * container = static_cast<ManagerTunnelWorker *>(ctx);
	std::lock_guard<std::mutex> lock(container->manager->mutex);
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
							"Socket is being closed" << std::endl;
	auto it = find(container->manager->activeTunnels.begin(),
				   container->manager->activeTunnels.end(),
				   container->localSocket);
	if( it == container->manager->activeTunnels.end()){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_WRN) <<
									"Socket is not active, something went wrong!" << std::endl;
		delete container->localSocket;

	}else{
		container->manager->activeTunnels.erase(it);
		//container->manager->freeTunnels.push_back(container->localSocket);
		delete container->localSocket;
	}
	container->worker->removeWork();
	container->sshConnection->deactivatedChannel();
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC) <<
			"Ended localSocketClose!" << std::endl;
}

void TunnelManager::stats(int fd, short event, void* arg) {
	struct timeval tv1;
	tv1.tv_sec = 30;
	tv1.tv_usec = 0;
	TunnelManager * manager = static_cast<TunnelManager*>(arg);
	std::lock_guard<std::mutex> lock(manager->mutex);

	int numCons = 0;
	for(auto connVec: manager->openConnections){
		for( auto con: std::get<1>(connVec)){
			OneInstanceLogger::instance().log("STAT",M_LOG_NRM, M_LOG_INF) <<
						"  Active connection:" <<
						*con << std::endl;
			numCons++;
		}
	}
	OneInstanceLogger::instance().log("STAT",M_LOG_NRM, M_LOG_INF) <<
				"Number of active connections destinations: " <<
				numCons << std::endl;
	OneInstanceLogger::instance().log("STAT",M_LOG_NRM, M_LOG_INF) <<
				"Number of active tunnels: " <<
				manager->activeTunnels.size() << std::endl;
	OneInstanceLogger::instance().log("STAT",M_LOG_NRM, M_LOG_INF) <<
				"Number of active unused Tunnels: " <<
				manager->freeTunnels.size() << std::endl;
	evtimer_add(manager->stats_event, static_cast<const timeval*>(&tv1));
}
void TunnelManager::cleanUp(int fd, short event, void* arg) {
	struct timeval tv1;
	tv1.tv_sec = 30;
	tv1.tv_usec = 0;
	TunnelManager * manager = static_cast<TunnelManager*>(arg);
	std::lock_guard<std::mutex> lock(manager->mutex);
	int totalSSHConnectionsClean = 0;
	for(auto connVector: manager->openConnections ){
		std::vector<tunnels::SSHConnection*> newVec;
		for( auto connection: std::get<1>(connVector)){
			if( connection->canBeRemoved() ){
				delete connection;
				totalSSHConnectionsClean++;
			}else{
				newVec.push_back(connection);
			}
		}
		manager->openConnections[std::get<0>(connVector)] = newVec;

	}
	OneInstanceLogger::instance().log("STAT",M_LOG_NRM, M_LOG_INF) <<
							" unused SSH connections!" << std::endl;
	evtimer_add(manager->cleanup_event, static_cast<const timeval*>(&tv1));
}
} /* namespace tunnelier */
