/*
 * Tunnel.hpp
 *
 *  Created on: Apr 2, 2014
 *      Author: joao
 */

#ifndef TUNNEL_HPP_
#define TUNNEL_HPP_
#include "Address.hpp"
#include "User.hpp"
class Tunnel {
public:
	/**
	 * Class constructor
	 * @param localPort Port to bind locally
	 * @param destination Destination address of the tunnel
	 * @param middle Middle machine address
	 * @param middleUser User to connect
	 */
	Tunnel( int localPort, Address destination, Address middle, User middleUser );
	virtual ~Tunnel();
private:
	int localPort;
	Address destination;
	Address middle;
	User middleUser;

};

#endif /* TUNNEL_HPP_ */
