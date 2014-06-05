/* EventAcceptor.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef EVENTACCEPTOR_HPP_
#define EVENTACCEPTOR_HPP_
#include <event.h>
namespace tunnelier {
namespace tunnels {

/**
 * Base class to all classes that work with events
 */
class EventAcceptor {
public:
	/**
	 * Class constructor
	 * @param base Pointer to the base of the events
	 */
	EventAcceptor(struct event_base *base);
	/**
	 * Class destructor
	 */
	virtual ~EventAcceptor();
	/**
	 * Retrieve the event base
	 */
	struct event_base *getEventBase();
	/**
	 * Set the event base
	 * @param new_base New base
	 */
	void setEventBase(struct event_base* new_base){
		this->base = new_base;
	};
protected:
	/**
	 * Current base
	 */
	struct event_base *base;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* EVENTACCEPTOR_HPP_ */
