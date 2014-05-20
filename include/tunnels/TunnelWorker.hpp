/*
 * TunnelWorker.hpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#ifndef TUNNELWORKER_HPP_
#define TUNNELWORKER_HPP_

#include "EventAcceptor.hpp"
#include <thread>
#include <mutex>
#include <event.h>
#include <iostream>
#include <unistd.h>

namespace tunnelier {
namespace tunnels {

class TunnelWorker: public EventAcceptor {
public:
	TunnelWorker();
	virtual ~TunnelWorker();
	TunnelWorker* start(){
		current_thread = new std::thread(&TunnelWorker::run, this);
		return this;
	}
	void stop_join(){
		keep_running = false;
		event_base_loopexit(base, nullptr);
		current_thread->join();
	}
	int getCurrentAmountOfWork(){
		std::lock_guard<std::mutex> lock(mutex);
		return amountOfWork;
	}
	static void run(TunnelWorker * worker){
		worker->base = event_base_new();

		worker->keep_running = true;
		while( worker->keep_running){
			event_base_dispatch(worker->base);
			sleep(1);
		}

	}
	inline void addWork(){
		std::lock_guard<std::mutex> lock(mutex);
		amountOfWork++;	
	};
	inline void removeWork(){
		std::lock_guard<std::mutex> lock(mutex);
		amountOfWork--;	
	};
private:
	std::thread* current_thread;
	int amountOfWork;
	bool keep_running;
	std::mutex mutex;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* TUNNELWORKER_HPP_ */
