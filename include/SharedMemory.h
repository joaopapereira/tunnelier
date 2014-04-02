/*
 * SharedMemory.h
 *
 *  Created on: Mar 29, 2014
 *      Author: joao
 */

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_
#include <vector>
#include <event.h>
#include "Tunnel.hpp"
class SharedMemory {
public:
	SharedMemory();
	virtual ~SharedMemory();
	struct event_base * getEventBase(){
		if(nullptr == base)
			base = event_base_new();
		return base;
	};
	struct event_base * getEventHTTPBase(){
			if(nullptr == http_base)
				http_base = event_base_new();
			return http_base;
		};
	void addTunnel(const std::string &name, Tunnel * tunnel);
private:
	struct event_base *http_base;
	struct event_base *base;
	std::vector<Tunnel*> tunnels;

};

#endif /* SHAREDMEMORY_H_ */
