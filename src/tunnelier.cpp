/*
 * tunnelier.cpp
 *
 *  Created on: Mar 27, 2014
 *      Author: joao
 */




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include "requestHandler.hpp"
#include "SharedMemory.h"
using namespace std;


int main(int argc, char **argv)
{
	string ip_addr = "0.0.0.0";
	int port =8080;
	SharedMemory *mem = new SharedMemory();
	RequestHandler rh(mem, ip_addr, port);
	mem->getEventBase();

	thread thr1 = rh.start_server();

	thr1.join();

	return 0;
}
