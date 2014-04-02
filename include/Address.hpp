/*
 * Address.hpp
 *
 *  Created on: Apr 2, 2014
 *      Author: joao
 */

#ifndef ADDRESS_HPP_
#define ADDRESS_HPP_
#include <string>

class Address{
public:
	/**
	 * Class constructor
	 * @param host Hostname or IP used to identify the address
	 * @param port Port of the address
	 */
	Address(std::string host, int port): host(host), port(port){};
	/**
	 * Retrieve the host of the current address
	 * @return Host
	 */
	std::string getHost(){
		return host;
	};
	/**
	 * Retrieve the port of the current address
	 * @return Port number
	 */
	int getPort(){
		return port;
	};

private:
	/**
	 * Hostname or IP of the address
	 */
	std::string host;
	/**
	 * Port number of the address
	 */
	int port;
};


#endif /* ADDRESS_HPP_ */
