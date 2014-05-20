/*
 * tunnelier.cpp
 *
 *  Created on: Mar 27, 2014
 *      Author: joao
 */




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <thread>
#include <mutex>
#include <event2/thread.h>
#include <event2/event.h>
#include "requestHandler.hpp"
#include "SharedMemory.h"
#include "tunnels/TunnelManager.hpp"
#include <libssh/callbacks.h>
#include "libJPLogger.hpp"
#include <getopt.h>
using namespace std;
using namespace jpCppLibs;
const int NUM_WORKERS = 1;
mutex mtx;
#define DEBUG 1

thread thr1;



void logger(int severity, const char *msg){
	std::lock_guard<std::mutex> lock(mtx);
	//cerr << "libevent [" << severity << "] " << msg << endl;
	OneInstanceLogger::instance().log("LIBEV",M_LOG_MAX, M_LOG_ERR) << severity << "] " << msg << endl;
}

int main(int argc, char **argv)
{
	OneInstanceLogger::instance().setFile("/tmp/tunnelier.log");
	OneInstanceLogger::instance().setLogLvl("ALL", M_LOG_MIN, M_LOG_ALLLVL);

	static struct option long_options[] =
	{
	    {"port", 2, NULL, 'p'},
	    {"host", 2, NULL, 'w'},
	    {NULL, 0, NULL, 0}
	};
	int option_index = 0;
	int c;
	string ip_addr = "0.0.0.0";
	int port =8080;
	while ((c = getopt_long(argc, argv, "p:w:",
				 long_options, &option_index)) != -1) {
		int this_option_optind = optind ? optind : 1;
		switch (c) {
		case 'p':
			port = std::stoi(optarg);
			break;
		case 'w':
			ip_addr = optarg;
			break;
		}
	}
	if (optind < argc) {
		printf ("non-option ARGV-elements: ");
		while (optind < argc)
			printf ("%s ", argv[optind++]);
		printf ("\n");
	}

	pid_t pid, sid;

   //Fork the Parent Process
	pid = fork();
	if (pid < 0) { exit(EXIT_FAILURE); }

	//We got a good pid, Close the Parent Process
	if (pid > 0) { exit(EXIT_SUCCESS); }

	//Change File Mask
	umask(0);

	//Create a new Signature Id for our child
	sid = setsid();
	if (sid < 0) { exit(EXIT_FAILURE); }

	//Change Directory
	//If we cant find the directory we exit with failure.
	if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

	//Close Standard File Descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	evthread_use_pthreads();
	event_set_log_callback(logger);
	event_enable_debug_mode();
	ssh_threads_set_callbacks(ssh_threads_get_noop());
	ssh_init();
	SharedMemory *mem = new SharedMemory();
	OneInstanceLogger::instance().log("APP",M_LOG_NRM, M_LOG_INF) << "Starting tunnelier!!!" << std::endl;
	tunnelier::TunnelManager * manager = new tunnelier::TunnelManager(NUM_WORKERS);
	RequestHandler rh(mem, manager, ip_addr, port);
	thr1 = rh.start_server();
	//manager->start();

	thr1.join();
	delete manager;
	OneInstanceLogger::instance().log("APP",M_LOG_NRM, M_LOG_INF,"Tunnelier stopped");
	return 0;
}
