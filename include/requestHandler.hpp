/* requestHandler.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef REQUESTHANDLER_HPP_
#define REQUESTHANDLER_HPP_
#include <string>
#include "libJPLogger.hpp"
#include "cxx11_implementations.hpp"
#include "SharedMemory.h"
#include "tunnels/TunnelManager.hpp"
/**
 * Class that will handle the requests
 */
class RequestHandler{
public:
	/**
	 * Class constructor
	 * @param memory At this moment not used
	 * @param manager Manager that will manage all the tunnels
	 * @param ip IP Address where the HTTP server will listen
	 * @param port Port where the HTTP server will listen
	 */
	RequestHandler(SharedMemory *memory, tunnelier::TunnelManager *manager, std::string ip, int port)
		:ip_address(ip),
		 port(port),
		 mem(memory),
		 manager(manager){
		mem = memory;
	};
	/**
	 * Starts the HTTP Server to listen for requests
	 */
	std::thread start_server();
	/**
	 * Instanciate the server
	 */
	int create_server();
	/**
	 * Retrieve the Manager
	 * @return Pointer to the manager
	 */
	inline tunnelier::TunnelManager * getManager(){
		return manager;
	}
	/**
	 * Retrieve the HTTP Event base
	 * @return HTTP Event base
	 */
	inline event_base * getHttpBase(){
		return mem->getEventBase();
	}
private:
	/**
	 * Shared memory
	 */
	SharedMemory *mem;
	/**
	 * Pointer to the Manager
	 */
	tunnelier::TunnelManager *manager;
	/**
	 * IP Address of the HTTP server
	 */
	std::string ip_address;
	/**
	 * Port of the HTTP Server
	 */
	int port;

};


#endif /* REQUESTHANDLER_HPP_ */
