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
	Address(std::string host, int port): host(host), port(port){};
	std::string getHost(){
		return host;
	};
	int getPort(){
		return port;
	};

private:
	std::string host;
	int port;
};


#endif /* ADDRESS_HPP_ */
