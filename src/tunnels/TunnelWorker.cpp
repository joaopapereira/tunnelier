/*
 * TunnelWorker.cpp
 *
 *  Created on: Apr 11, 2014
 *      Author: joao
 */

#include "tunnels/TunnelWorker.hpp"

namespace tunnelier {
namespace tunnels {

TunnelWorker::TunnelWorker() :
	EventAcceptor(event_base_new()){
	// TODO Auto-generated constructor stub

}

TunnelWorker::~TunnelWorker() {
	// TODO Auto-generated destructor stub
}

} /* namespace tunnels */
} /* namespace tunnelier */
