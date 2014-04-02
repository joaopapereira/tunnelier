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
#include "Tunnel.hpp"
#include "Address.hpp"
#include "User.hpp"
class RequestHandler{
public:
	RequestHandler(SharedMemory *memory, std::string ip, int port)
		:ip_address(ip), port(port){
		mem = memory;
	};
	std::thread start_server();
	int create_server();
private:

	SharedMemory *mem;
	std::string ip_address;
	int port;

};


#endif /* REQUESTHANDLER_HPP_ */
