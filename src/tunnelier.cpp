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
#include <mutex>
#include <event2/thread.h>
#include <event2/event.h>
#include "requestHandler.hpp"
#include "SharedMemory.h"
#include "tunnels/TunnelManager.hpp"
using namespace std;

const int NUM_WORKERS = 5;
mutex mtx;

void logger(int severity, const char *msg){
	std::lock_guard<std::mutex> lock(mtx);
	cerr << "libevent [" << severity << "] " << msg << endl;
}

int main(int argc, char **argv)
{
	string ip_addr = "0.0.0.0";
	int port =8080;
	evthread_use_pthreads();
	event_set_log_callback(logger);
	event_enable_debug_mode();
	SharedMemory *mem = new SharedMemory();

	tunnelier::TunnelManager * manager = new tunnelier::TunnelManager(NUM_WORKERS);
	RequestHandler rh(mem, manager, ip_addr, port);
	thread thr1 = rh.start_server();
	//manager->start();

	thr1.join();
	delete manager;
	return 0;
}
