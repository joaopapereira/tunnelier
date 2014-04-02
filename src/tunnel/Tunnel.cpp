/*
 * Tunnel.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: joao
 */

#include "Tunnel.hpp"

Tunnel::Tunnel(int localPort, Address destination, Address middle, User middleUser):
	localPort(localPort),
	destination(destination),
	middle(middle),
	middleUser(middleUser){

}

Tunnel::~Tunnel() {
	// TODO Auto-generated destructor stub
}

