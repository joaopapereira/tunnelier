/*
 * TunnelManager.cpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#include "tunnels/TunnelManager.hpp"
#include <algorithm>
#include <iostream>
#include <utility>


namespace tunnelier {
std::string TunnelManager::LOGNAME("MAN");
using namespace tunnels;
using namespace jpCppLibs;
#ifdef USE_BOOST_INSTEAD_CXX11
using namespace boost;
#else
using namespace std;
#endif

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
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::mutex*> lock(&mutex);
#else
	std::lock_guard<std::mutex> lock(mutex);
#endif
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC,"Destroying tunnel manager" );
	// TODO Auto-generated destructor stub
	event_del(poll_event);
	event_del(stats_event);
	event_del(cleanup_event);
#ifdef USE_BOOST_INSTEAD_CXX11
	for(std::map<int, tunnels::SocketListener*>::iterator it = openListeners.begin();
	    it != openListeners.end();
	    it++)
		delete (*it).second;
#else
	for(auto t: openListeners){
		delete std::get<1>(t);
	}
#endif


#ifdef USE_BOOST_INSTEAD_CXX11
	for(std::vector<tunnels::TunnelWorker * >::iterator it = workers.begin();
	    it != workers.end();
	    it++){
                tunnels::TunnelWorker * w = *it;
#else
	for(tunnels::TunnelWorker * w: workers){
#endif
		w->stop_join();
		delete w;
	}
	workers.clear();
#ifdef USE_BOOST_INSTEAD_CXX11
	for(std::vector<tunnels::LocalTunnelSSH*>::iterator it = activeTunnels.begin();
	    it != activeTunnels.end();
	    it++)
                delete *it;
#else
	for(auto tunnel: activeTunnels){
		delete tunnel;
	}
#endif

	activeTunnels.clear();
#ifdef USE_BOOST_INSTEAD_CXX11
	for(std::vector<tunnels::LocalTunnelSSH*>::iterator it = freeTunnels.begin();
	    it != freeTunnels.end();
	    it++)
                delete *it;
#else
	for(auto tunnel: freeTunnels){
		delete tunnel;
	}
#endif

	freeTunnels.clear();

	event_free(poll_event);
	event_free(stats_event);
	event_free(cleanup_event);
}



int TunnelManager::createTunnel(int localPort, Address middleAddress,
		User middleUser, Address destination) {
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::mutex*> lock(&mutex);
#else
	std::lock_guard<std::mutex> lock(mutex);
#endif
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_TRC,"Starting tunnel creation!");
	if( 0 == createListener(localPort, destination) ){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG,"Check if connection already open");
		if( 0 == isSSHConnectionOpen( middleAddress, middleUser) ){
			OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF,"Tunnel already exists!");
			return 0;
		}
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG,"Create SSH Connection!");
		if( 0 == createSSHConnection( middleAddress, middleUser) ){
			tunnelLink[localPort] = make_tuple(middleAddress, middleUser, destination);
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
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::mutex*> lock(&mutex);
#else
	std::lock_guard<std::mutex> lock(mutex);
#endif
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_INF,"Closing tunnel at: %d!", localPort);

	try{
		tunnels::SocketListener* listener = openListeners.at(localPort);
		openListeners.erase(localPort);
		delete listener;
	}catch(...){ return -1;};
	try{
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
#ifdef USE_BOOST_INSTEAD_CXX11
	openConnections.insert(std::make_pair<std::pair<Address,User>,std::vector<tunnels::SSHConnection*> >(std::make_pair<Address, User>(host,user),std::vector<tunnels::SSHConnection*>()));
#else
	openConnections.insert(std::make_pair<tuple<Address,User>,std::vector<tunnels::SSHConnection*> >(make_tuple(host,user),std::vector<tunnels::SSHConnection*>()));
#endif
	SSHConnection * connection = new SSHConnection(host, user);
	if( 0 == connection->connect() ){
		OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
								"Connection created with success" << std::endl;

#ifdef USE_BOOST_INSTEAD_CXX11
		openConnections[std::make_pair<Address,User>(host,user)].push_back(connection);
#else
		openConnections[make_tuple(host,user)].push_back(connection);
#endif
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
#ifdef USE_BOOST_INSTEAD_CXX11
		std::vector<tunnels::SSHConnection*>  vec = openConnections.at(std::make_pair<Address,User>(host,user));
#else
		std::vector<tunnels::SSHConnection*>  vec = openConnections.at(make_tuple(host,user));
#endif
		return vec.size();
	}catch(std::out_of_range &e){}
	catch(...){}
	return -1;
}

bool compareWorker( TunnelWorker *a , TunnelWorker *b){
	return b->getCurrentAmountOfWork() > a->getCurrentAmountOfWork();
}
TunnelWorker* TunnelManager::nextAvailableWorker() {
	sort(workers.begin(), workers.end(), compareWorker);
	//sort(workers.begin(), workers.end(), [](TunnelWorker * a, TunnelWorker * b) {
	//	return b->getCurrentAmountOfWork() > a->getCurrentAmountOfWork();
	//});
	return *workers.begin();
}


void TunnelManager::poolTunnels(int fd, short event, void* arg) {
	TunnelManager * manager = static_cast<TunnelManager*>(arg);
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::mutex*> lock(&(manager->mutex));
#else
	std::lock_guard<std::mutex> lock(manager->mutex);
#endif

#ifdef USE_BOOST_INSTEAD_CXX11
	for(std::vector<tunnels::LocalTunnelSSH*>::iterator it = manager->activeTunnels.begin();
	    it != manager->activeTunnels.end();
	    it++){
		tunnels::LocalTunnelSSH* tunnel = *it;
#else
	for(auto tunnel: manager->activeTunnels){
#endif
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
	tuple<Address,User,Address>  res = manager->tunnelLink[listen->getLocalPort()];
	SSHRemoteEndPoint endpoint(get<0>(res), get<1>(res), get<2>(res));
	TunnelWorker * worker = manager->nextAvailableWorker();
	TunnelWorker * w1;
	//auto it = std::find(manager->freeTunnels.begin(), manager->freeTunnels.end(),&endpoint);
	tunnels::LocalTunnelSSH * sshTunnel = nullptr;
	std::vector<tunnels::LocalTunnelSSH*>::iterator  delCursor = manager->freeTunnels.begin();
#ifdef USE_BOOST_INSTEAD_CXX11
	for(std::vector<tunnels::LocalTunnelSSH*>::iterator iter = manager->freeTunnels.begin();
	    iter != manager->freeTunnels.end();
	    iter++){
		tunnels::LocalTunnelSSH* it = *iter;
#else
	for(auto it: manager->freeTunnels ){
#endif
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
#ifdef USE_BOOST_INSTEAD_CXX11
			std::vector<tunnels::SSHConnection*> allConnections = manager->openConnections.at(std::make_pair<Address,User>(get<0>(res), get<1>(res)));
			for(std::vector<tunnels::SSHConnection*>::iterator iter = allConnections.begin();
			    iter != allConnections.end();
			    iter++){
				tunnels::SSHConnection* t1 = *iter;
#else
			std::vector<tunnels::SSHConnection*> allConnections = manager->openConnections.at(make_tuple(get<0>(res), get<1>(res)));
			for( auto t1: allConnections){
#endif
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
				conn = new tunnels::SSHConnection(get<0>(res), get<1>(res));
				if( 0 == conn->connect() ){
					OneInstanceLogger::instance().log(LOGNAME,M_LOG_NRM, M_LOG_DBG) <<
											"Connection created with success" << std::endl;
					OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
									"Connection created with success" << std::endl;
					conn->setPollCallBack(TunnelManager::poolTunnels, manager, worker->getEventBase());
					worker->addWork();
#ifdef USE_BOOST_INSTEAD_CXX11
					manager->openConnections[std::make_pair<Address,User>(get<0>(res), get<1>(res))].push_back(conn);
#else
					manager->openConnections[make_tuple(get<0>(res), get<1>(res))].push_back(conn);
#endif
				}else
					throw std::ios_base::failure("Unable to create new connection");
			}
			OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
									"Going to create End Point" << std::endl;
			sshEndPoint = conn->createEndPoint(get<2>(res), worker->getEventBase());
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
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::mutex*> lock(&(container->manager->mutex));
#else
	std::lock_guard<std::mutex> lock(container->manager->mutex);
#endif
	OneInstanceLogger::instance().log(LOGNAME,M_LOG_LOW, M_LOG_DBG) <<
							"Socket is being closed" << std::endl;
	std::vector<tunnels::LocalTunnelSSH*>::iterator it = find(container->manager->activeTunnels.begin(),
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
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::mutex*> lock(&(manager->mutex));
#else
	std::lock_guard<std::mutex> lock(manager->mutex);
#endif

	int numCons = 0;
#ifdef USE_BOOST_INSTEAD_CXX11
	for(std::map<std::pair<Address,User>,std::vector<tunnels::SSHConnection*> >::iterator iter = manager->openConnections.begin();
	    iter != manager->openConnections.end();
	    iter++){
		std::vector<tunnels::SSHConnection*> connVec = iter->second;
		for(std::vector<tunnels::SSHConnection*>::iterator iter1 = (connVec).begin();
		    iter1 != (connVec).end();
		    iter1++){
			tunnels::SSHConnection* con = *iter1;
#else
	for(auto connVec: manager->openConnections){
		for( auto con: std::get<1>(connVec)){
#endif
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
#ifdef USE_BOOST_INSTEAD_CXX11
	std::lock_guard<std::mutex*> lock(&(manager->mutex));
#else
	std::lock_guard<std::mutex> lock(manager->mutex);
#endif
	int totalSSHConnectionsClean = 0;
#ifdef USE_BOOST_INSTEAD_CXX11
	for(std::map<std::pair<Address,User>,std::vector<tunnels::SSHConnection*> >::iterator iter = manager->openConnections.begin();
	    iter != manager->openConnections.end();
	    iter++){
		std::vector<tunnels::SSHConnection*> connVector = iter->second;
		std::vector<tunnels::SSHConnection*> newVec;
		for(std::vector<tunnels::SSHConnection*>::iterator iter1 = (connVector).begin();
		    iter1 != (connVector).end();
		    iter1++){
			tunnels::SSHConnection* connection = *iter1;
#else
	for(auto connVector: manager->openConnections ){
		std::vector<tunnels::SSHConnection*> newVec;
		for( auto connection: std::get<1>(connVector)){
#endif
			if( connection->canBeRemoved() ){
				delete connection;
				totalSSHConnectionsClean++;
			}else{
				newVec.push_back(connection);
			}
		}
#ifdef USE_BOOST_INSTEAD_CXX11
		manager->openConnections[iter->first] = newVec;
#else
		manager->openConnections[get<0>(connVector)] = newVec;
#endif

	}
	OneInstanceLogger::instance().log("STAT",M_LOG_NRM, M_LOG_INF) <<
							" unused SSH connections!" << std::endl;
	evtimer_add(manager->cleanup_event, static_cast<const timeval*>(&tv1));
}
} /* namespace tunnelier */
