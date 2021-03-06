/* Module.hpp --
 *
 * Copyright (c) 2014 Joao Pereira <joaopapereira@gmail.com>
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef MODULE_HPP_
#define MODULE_HPP_
#include <string>
#include <stdarg.h>
#include "libJPLogger.hpp"
class Module{
public:
/*
	void log(int logsev, int type,std::string message , ...){
		va_list args;
		va_start( args, message );
		jpCppLibs::OneInstanceLogger::instance().log(LOGNAME, logsev, type, args);
		va_end( args );
	};*/
protected:
	std::string LOGNAME;
};



#endif /* MODULE_HPP_ */
