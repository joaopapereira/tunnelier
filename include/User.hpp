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
	/**
	 * Class constructor
	 * @param username Username to use why connecting
	 * @param password Password of the user
	 */
	User(std::string username, std::string password): username(username), password(password){};
	/**
	 * Retrieve the username
	 * @return Username
	 */
	std::string getUsername(){
		return username;
	};
	/**
	 * Retrieve the password for the user
	 * @return Password
	 */
	std::string getPassword(){
		return password;
	};
private:
	/**
	 * Username
	 */
	std::string username;
	/**
	 * Password
	 */
	std::string password;
};



#endif /* USER_HPP_ */
