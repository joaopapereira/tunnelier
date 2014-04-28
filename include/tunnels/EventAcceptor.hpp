/*
 * EventAcceptor.hpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#ifndef EVENTACCEPTOR_HPP_
#define EVENTACCEPTOR_HPP_
#include <event.h>
namespace tunnelier {
namespace tunnels {

class EventAcceptor {
public:
	EventAcceptor(struct event_base *base);
	virtual ~EventAcceptor();
	struct event_base *getEventBase();
	void setEventBase(struct event_base* new_base){
		this->base = new_base;
	};
protected:
	struct event_base *base;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* EVENTACCEPTOR_HPP_ */
