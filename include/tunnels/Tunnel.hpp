/*
 * Tunnel.hpp
 *
 *  Created on: Apr 10, 2014
 *      Author: joao
 */

#ifndef TUNNEL_HPP_
#define TUNNEL_HPP_

namespace tunnelier {
namespace tunnels {
class EndPoint;
class Tunnel{
public:
	Tunnel(EndPoint * local, EndPoint *remote): local(local), remote(remote){};
	virtual ~Tunnel(){};
	inline EndPoint * getLocalEndPoint(){
		return local;
	};
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
