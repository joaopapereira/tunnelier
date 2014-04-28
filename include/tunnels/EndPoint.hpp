/*
 * EndPoint.hpp
 *
 *  Created on: Apr 10, 2014
 *      Author: joao
 */

#ifndef ENDPOINT_HPP_
#define ENDPOINT_HPP_

#include <string.h>
namespace tunnelier {
namespace tunnels {
const int MAX_READ_SIZE = 2048;
class EndPoint {
public:
	EndPoint();
	virtual ~EndPoint();
	virtual int poll() = 0;
	int moveDataTo(EndPoint * otherEndpoint){
		if( length == 0 )
			return 0;
		//int amountWritten = otherEndpoint->writeToEndPoint(data, length);
		data[0] = '\0';
		length = 0;
		//return amountWritten;
	};
	inline int directCopy(EndPoint * otherEndpoint, void * data, int length){
		if( length == 0 )
			return 0;
		//int amountWritten = otherEndpoint->writeToEndPoint(data, length);
		//return amountWritten;
	}
	inline void setData( void* data, int length){
		memcpy(this->data, data, length);
		this->length = length;
	};
protected:
	virtual int readFromEndPoint() = 0;

	char data[MAX_READ_SIZE];
	int length;
};

} /* namespace tunnels */
} /* namespace tunnelier */

#endif /* ENDPOINT_HPP_ */
