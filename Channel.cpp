#include "Channel.hpp"

///////////////////////////////////////////////////////////////////////////////
//  Forbidden :
Channel::Channel() {}
Channel::Channel(Channel const& source) { (void)source; }
Channel& Channel::operator=(Channel const& source) { (void)source; return (*this); }
///////////////////////////////////////////////////////////////////////////////

Channel::Channel(const std::string& name, User *user) :
_name(name), _limit(NOT_SET), _flagInvite(false), _flagPassword(false), _flagTopic(false)
{
	addUser(user);
	addOpe(user);
	std::cout << "New channel \"" << _name << "\" is created" << std::endl;
}

Channel::~Channel()
{
	std::cout << "Channel \"" << _name << "\" is destroyed" << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// Getters

std::string Channel::get_password() const
{
	return (_password);
}

std::string Channel::get_topic() const
{
	return (_topic);
}

int	Channel::getLimit() const
{
	return (_limit);
}

bool	Channel::get_flagInvite() const
{
	return (_flagInvite);
}

bool	Channel::get_flagPassword() const
{
	return (_flagPassword);
}

bool	Channel::get_flagTopic() const
{
	return (_flagTopic);
}

///////////////////////////////////////////////////////////////////////////////
// Setters

void	Channel::set_password(std::string password)
{
	_password = password;
}

void	Channel::set_topic(std::string topic)
{
	_topic = topic;
}

void	Channel::setLimit(int limit)
{
	_limit = limit;
}

void	Channel::set_flagInvite(bool flag)
{
	_flagInvite = flag;
}

void	Channel::set_flagPassword(bool flag)
{
	_flagPassword = flag;
}

void	Channel::set_flagTopic(bool flag)
{
	_flagTopic = flag;
}

///////////////////////////////////////////////////////////////////////////////
// Checkers

bool	Channel::check_if_ope(User *user) const
{
	std::vector<User*>::const_iterator it = std::find(_reg_ope.begin(), _reg_ope.end(), user);
	if (it != _reg_ope.end())
		return (true);
	return (false);
}

bool	Channel::check_if_user(User* user) const
{
	std::vector<User*>::const_iterator it = std::find(_reg_users.begin(), _reg_users.end(), user);
	if (it != _reg_users.end())
		return (true);
	return (false);
}

bool	Channel::check_if_inv(std::string username) const
{
	std::vector<std::string>::const_iterator it = std::find(_reg_inv.begin(), _reg_inv.end(), username);
	if (it != _reg_inv.end())
		return (true);
	return (false);
}

bool	Channel::check_if_empty() const
{
	std::vector<User*>::const_iterator it = _reg_users.begin();
	it = _reg_ope.begin();
	if (it == _reg_ope.end())
		return (true);
	return (false);
}

bool	Channel::check_if_space_available()
{
	if (_limit == NOT_SET || user_counter() < _limit)
		return (true);
	return (false);
}

///////////////////////////////////////////////////////////////////////////////
// Methods

bool	Channel::addUser(User *user)
{
	if (!check_if_user(user))
	{
		if (check_if_space_available())
		{
			_reg_users.push_back(user);
			return (true);
		}
		else
		{
			send(user->get_userSocket(), "Error : the channel is full.\n", strlen("Error : the channel is full.\n"), 0);
			std::cout << "channel is full. adding new user aborted" << std::endl;
		}
	}
	return (false);
}

bool	Channel::addOpe(User *user)
{
	if (!check_if_ope(user))
	{
		if (check_if_user(user))
		{
			_reg_ope.push_back(user);
			return (true);
		}
		else
		{
			send(user->get_userSocket(), "Error : client is supposed to be user in order to be also operator\n", strlen("Error : client is supposed to be user in order to be also operator\n"), 0);
			std::cout << "user cannot become ope without being a user" << std::endl;
		}
	}
	return (false);
}

bool	Channel::addInv(std::string const username)
{
	if (!check_if_inv(username))
	{
		_reg_inv.push_back(username);
		return (true);
	}
	return (false);
}

void	Channel::unset_flagInvite()
{
	set_flagInvite(false);
	_reg_inv.clear();
}

void	Channel::unset_flagPassword()
{
	set_flagPassword(false);
	set_password("");
}

void Channel::sendMessage(const std::string& message, type_sock sender_socket)
{
	for (std::vector<User*>::iterator it = _reg_users.begin(); it != _reg_users.end(); it++)
	{
		if ((*it)->get_userSocket() != sender_socket)
			send((*it)->get_userSocket(), message.c_str(), message.size(), 0);
	}
}

int	Channel::user_counter()
{
	int i = 0;
	for (std::vector<User*>::iterator it = _reg_users.begin(); it != _reg_users.end(); it++)
		i++;
	std::cout << "there is currently " << i << " users in channel #" << _name << std::endl;
	return (i);
}

///////////////////////////////////////////////////////////////////////////////
// Cleaners

void	Channel::clear_topic()
{
	_topic.clear();
}

bool	Channel::removeOpe(User *user)
{
	std::vector<User*>::iterator ope_it = std::find(_reg_ope.begin(), _reg_ope.end(), user);
	if (ope_it != _reg_ope.end())
	{
		_reg_ope.erase(ope_it);
		return (true);
	}
	return (false);
}

bool	Channel::removeUser(User *user)
{
	std::vector<User*>::iterator ope_it = std::find(_reg_ope.begin(), _reg_ope.end(), user);
	std::vector<User*>::iterator user_it = std::find(_reg_users.begin(), _reg_users.end(), user);
	if (ope_it != _reg_ope.end())
		_reg_ope.erase(ope_it);
	if (user_it != _reg_users.end())
	{
		_reg_users.erase(user_it);
		std::cout << "user removed from channel #" << _name << std::endl;
		return (true);
	}
	return (false);
}

bool	Channel::removeInv(std::string const username)
{
	std::vector<std::string>::iterator user_it = std::find(_reg_inv.begin(), _reg_inv.end(), username);
	if (user_it != _reg_inv.end())
	{
		_reg_inv.erase(user_it);
		std::cout << "user removed from invitation list of channel #" << _name << std::endl;
		return (true);
	}
	return (false);
}
