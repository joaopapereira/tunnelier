/* TunnelWorker.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef TUNNELWORKER_HPP_
#define TUNNELWORKER_HPP_

#include "cxx11_implementations.hpp"
#include "EventAcceptor.hpp"
#include <event.h>
#include <iostream>
#include <unistd.h>

namespace tunnelier {
namespace tunnels {
/**
 * Working thread
 */
class TunnelWorker: public EventAcceptor {
public:
	/**
	 * Class constructor
	 */
	TunnelWorker();
	/**
	 * Class destructor
	 */
	virtual ~TunnelWorker();
	/**
	 * Start the worker
	 */
	TunnelWorker* start(){
		current_thread = new std::thread(&TunnelWorker::run, this);
		return this;
	}
	/**
	 * Stop the worker
	 */
	void stop_join(){
		keep_running = false;
		event_base_loopexit(base, nullptr);
		current_thread->join();
	}
	/**
	 * Check the amount of work the worker have
	 */
	int getCurrentAmountOfWork(){
#ifdef USE_BOOST_INSTEAD_CXX11
		std::lock_guard<std::mutex*> lock(&mutex);
#else
		std::lock_guard<std::mutex> lock(mutex);
#endif
		return amountOfWork;
	}
	/**
	 * Function to be run for the life of the worker
	 * @param worker Worker itself
	 */
	static void run(TunnelWorker * worker){
		worker->base = event_base_new();

		worker->keep_running = true;
		while( worker->keep_running){
			event_base_dispatch(worker->base);
			sleep(1);
		}

	}
	/**
	 * Add work to the worker
	 */
	inline void addWork(){
#ifdef USE_BOOST_INSTEAD_CXX11
		std::lock_guard<std::mutex*> lock(&mutex);
#else
		std::lock_guard<std::mutex> lock(mutex);
#endif
		amountOfWork++;	
	};
	/**
	 * Remove work
	 */
	inline void removeWork(){
#ifdef USE_BOOST_INSTEAD_CXX11
		std::lock_guard<std::mutex*> lock(&mutex);
#else
		std::lock_guard<std::mutex> lock(mutex);
#endif
		amountOfWork--;	
	};
private:
	/**
	 * Thread
	 */
	std::thread* current_thread;
	/**
	 * Current amount of work
	 */
	int amountOfWork;
	/**
	 * Keep the worker running or not
	 */
	bool keep_running;
	std::mutex mutex;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* TUNNELWORKER_HPP_ */
