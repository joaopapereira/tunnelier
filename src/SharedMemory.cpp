/*
 * SharedMemory.cpp
 *
 *  Created on: Mar 29, 2014
 *      Author: joao
 */

#include "SharedMemory.h"
#include <iostream>

SharedMemory::SharedMemory()
	:base(nullptr),
	 tunnels(){
	// TODO Auto-generated constructor stub
	tunnels = {};
}

SharedMemory::~SharedMemory() {
	// TODO Auto-generated destructor stub
}

void
SharedMemory::addTunnel(const std::string &name, Tunnel * tunnel){
	std::cout << "Adding " << name << std::endl;
	tunnels.push_back(tunnel);
}
