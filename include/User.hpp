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
	User(){};
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
	/**
	 * Compare two users
	 * @param rhs User to compare against
	 * @return true if they are equal
	 *         false otherwise
	 */
	inline bool operator==(const User& rhs){
		return (0==username.compare(rhs.username)) &&
			   (0==password.compare(rhs.password));
	};
	/**
	 * Compare two users
	 * @param rhs User to compare against
	 * @return true if they are equal
	 *         false otherwise
	 */
	friend inline bool operator< (const User & s1, const User & s2){

		return (s1.username.size()+s1.password.size()) <
				(s2.username.size()+s2.password.size());
	};
	friend std::ostream& operator<<(std::ostream& os, const User& user)
	{
	  os << user.username;
	  return os;
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
