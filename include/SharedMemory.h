/* SharedMemory.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_
#include <vector>
#include <event.h>
#include "cxx11_implementations.hpp"
//#include "Tunnel.hpp"
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
	//void addTunnel(const std::string &name, Tunnelv1 * tunnel);
private:
	struct event_base *http_base;
	struct event_base *base;
	//std::vector<Tunnelv1*> tunnels;

};

#endif /* SHAREDMEMORY_H_ */
