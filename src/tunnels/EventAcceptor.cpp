/*
 * EventAcceptor.cpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#include "tunnels/EventAcceptor.hpp"

namespace tunnelier {
namespace tunnels {

EventAcceptor::EventAcceptor(struct event_base* base) :
	base(base){
}

struct event_base*
EventAcceptor::getEventBase() {
	return base;
}

EventAcceptor::~EventAcceptor() {
	// TODO Auto-generated destructor stub
}

} /* namespace tunnels */
} /* namespace tunnelier */


