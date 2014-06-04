/* EndPoint.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef ENDPOINT_HPP_
#define ENDPOINT_HPP_
#include "Module.hpp"
#include <string.h>
namespace tunnelier {
namespace tunnels {
const int MAX_READ_SIZE = 2048;
/**
 * Class that will store one End Point for the tunnel
 */
class EndPoint: public Module {
public:
	/**
	 * Class Constructor
	 */
	EndPoint();
	/**
	 * Class destructor
	 */
	virtual ~EndPoint();
	/**
	 * Function to be overloaded
	 * Poll the End Point to see if it as data to be transfered
	 * @return Amount of data to be transfered
	 */
	virtual int poll() = 0;


protected:
	/**
	 * Function to be overloaded
	 * Read data from the current end point
	 * @return Amount of data readed
	 */
	virtual int readFromEndPoint() = 0;
	/**
	 * Data
	 */
	char data[MAX_READ_SIZE];
	/**
	 * Amount of data readed
	 */
	int length;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* ENDPOINT_HPP_ */
