/*
 * SharedMemory.h
 *
 *  Created on: Mar 29, 2014
 *      Author: joao
 */

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_
#include <map>
class SharedMemory {
public:
	SharedMemory();
	virtual ~SharedMemory();
private:
	std::map<std::string,int> test;
};

#endif /* SHAREDMEMORY_H_ */
