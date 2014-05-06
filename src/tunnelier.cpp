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
#include "libJPLogger.hpp"
using namespace std;
using namespace jpCppLibs;
const int NUM_WORKERS = 5;
mutex mtx;
#define DEBUG 1

void logger(int severity, const char *msg){
	std::lock_guard<std::mutex> lock(mtx);
	//cerr << "libevent [" << severity << "] " << msg << endl;
	OneInstanceLogger::instance().log("LIBEV",M_LOG_MAX, M_LOG_ERR) << severity << "] " << msg << endl;
}

int main(int argc, char **argv)
{
	OneInstanceLogger::instance().setFile("/tmp/tunnelier.log");
	OneInstanceLogger::instance().setLogLvl("ALL", M_LOG_MIN, M_LOG_ALLLVL);
	string ip_addr = "0.0.0.0";
	int port =8080;
	evthread_use_pthreads();
	event_set_log_callback(logger);
	event_enable_debug_mode();
	SharedMemory *mem = new SharedMemory();
	OneInstanceLogger::instance().log("APP",M_LOG_NRM, M_LOG_INF) << "Starting tunnelier!!!" << std::endl;
	tunnelier::TunnelManager * manager = new tunnelier::TunnelManager(NUM_WORKERS);
	RequestHandler rh(mem, manager, ip_addr, port);
	thread thr1 = rh.start_server();
	//manager->start();

	thr1.join();
	delete manager;
	OneInstanceLogger::instance().log("APP",M_LOG_NRM, M_LOG_INF,"Tunnelier stopped");
	return 0;
}
