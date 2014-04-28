/*
 * requestHandler.hpp
 *
 *  Created on: Mar 29, 2014
 *      Author: joao
 */

#ifndef REQUESTHANDLER_HPP_
#define REQUESTHANDLER_HPP_
#include <string>
#include <thread>
#include "SharedMemory.h"
#include "tunnels/TunnelManager.hpp"
class RequestHandler{
public:
	RequestHandler(SharedMemory *memory, tunnelier::TunnelManager *manager, std::string ip, int port)
		:ip_address(ip),
		 port(port),
		 mem(memory),
		 manager(manager){
		mem = memory;
	};
	std::thread start_server();
	int create_server();
	inline tunnelier::TunnelManager * getManager(){
		return manager;
	}
private:

	SharedMemory *mem;
	tunnelier::TunnelManager *manager;
	std::string ip_address;
	int port;

};


#endif /* REQUESTHANDLER_HPP_ */
