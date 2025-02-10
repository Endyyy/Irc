#include "User.hpp"

////////////////////////////////////////////////////////////////////////////////
//  Forbidden :
User::User() : _userSocket(0) {}
User::User(User const& source) : _userSocket(0) { (void)source; }
User& User::operator=(User const& source) { (void)source; return (*this); }
////////////////////////////////////////////////////////////////////////////////

User::User(type_sock const socket) : _userSocket(socket), _userState(0)
{
	std::cout << "New User created. User socket is " << _userSocket << std::endl;
}

User::~User()
{
	std::cout << "A user has been destroyed" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// Getters

std::string User::get_nickname() const
{
	return (_nickname);
}

std::string User::get_username() const
{
	return (_username);
}

type_sock	User::get_userSocket() const
{
	return (_userSocket);
}

int			User::get_userState() const
{
	return (_userState);
}

std::string	User::get_inputStack() const
{
	return (_inputStack);
}

////////////////////////////////////////////////////////////////////////////////
// Setters

void	User::set_username(std::string username)
{
	_username = username;
}

void	User::set_nickname(std::string nickname)
{
	_nickname = nickname;
}

void	User::set_userState(int state)
{
	_userState = state;
}

void	User::set_inputStack(std::string str)
{
	_inputStack = str;
}

////////////////////////////////////////////////////////////////////////////////
// Methods

void	User::add_to_inputStack(std::string str)
{
	set_inputStack(get_inputStack() + str);
}

////////////////////////////////////////////////////////////////////////////////
// Cleaners

void	User::reset_inputStack()
{
	set_inputStack("");
}
