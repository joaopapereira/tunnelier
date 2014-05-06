/*
 * Address.hpp
 *
 *  Created on: Apr 2, 2014
 *      Author: joao
 */

#ifndef ADDRESS_HPP_
#define ADDRESS_HPP_
#include <string>
#include <iostream>

class Address{
public:
	Address(){};
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
	/**
	 * Compare two users
	 * @param rhs User to compare against
	 * @return true if they are equal
	 *         false otherwise
	 */
	inline bool operator==(const Address& rhs){
		return (0==host.compare(rhs.host)) &&
			   (port==rhs.port);
	};
	/**
	 * Compare two users
	 * @param rhs User to compare against
	 * @return true if they are equal
	 *         false otherwise
	 */
	friend inline bool operator< (const Address & s1, const Address & s2){

		return (s1.host.size()+s1.port) <
				(s2.host.size()+s2.port);
	};
	friend std::ostream& operator<<(std::ostream& os, const Address& addr){
	  return os << addr.host << ":" << addr.port;
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
