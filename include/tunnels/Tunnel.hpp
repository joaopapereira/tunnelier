/* Tunnel.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef TUNNEL_HPP_
#define TUNNEL_HPP_

namespace tunnelier {
namespace tunnels {
class EndPoint;
/**
 * Tunnel base class
 */
class Tunnel{
public:
	/**
	 * Class constructor
	 */
	Tunnel(EndPoint * local, EndPoint *remote): local(local), remote(remote){};
	/**
	 * class destructor
	 */
	virtual ~Tunnel(){};
	/**
	 * Retrieve local end point
	 */
	inline EndPoint * getLocalEndPoint(){
		return local;
	};
	/**
	 * Retrieve remote end point
	 */
	inline EndPoint * getRemoteEndPoint(){
		return remote;
	};
protected:
	EndPoint *local;
	EndPoint *remote;
};
} /* namespace tunnels */
} /* namespace tunnelier */



#endif /* TUNNEL_HPP_ */
