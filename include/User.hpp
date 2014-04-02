/*
 * User.hpp
 *
 *  Created on: Apr 2, 2014
 *      Author: joao
 */

#ifndef USER_HPP_
#define USER_HPP_

class User{
public:
	User(std::string username, std::string password): username(username), password(password){};
	std::string getUsername(){
		return username;
	};
	std::string getPassword(){
		return password;
	};
private:
	std::string username;
	std::string password;
};



#endif /* USER_HPP_ */
