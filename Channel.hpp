#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include "User.hpp"

# include <algorithm>
# include <arpa/inet.h>
# include <cstring>
# include <iostream>
# include <vector>

# define NOT_SET -1

typedef int type_sock;

class Channel
{
	private:
		std::string	_name;
		std::string	_topic;
		std::string	_password;
		int			_limit;
		bool		_flagInvite;
		bool		_flagPassword;
		bool		_flagTopic;

		std::vector<User*>			_reg_users;
		std::vector<User*>			_reg_ope;
		std::vector<std::string>	_reg_inv;

		Channel();
		Channel(Channel const& source);
		Channel& operator=(Channel const& source);

	public:
		Channel(const std::string& name, User *user);
		~Channel();

		// Getters
		std::string	get_password() const;
		std::string	get_topic() const;
		int			getLimit() const;
		bool		get_flagInvite() const;
		bool		get_flagPassword() const;
		bool		get_flagTopic() const;

		// Setters
		void	set_password(std::string password);
		void	set_topic(std::string topic);
		void	setLimit(int limit);
		void	set_flagInvite(bool state);
		void	set_flagPassword(bool state);
		void	set_flagTopic(bool state);

		// Checkers
		bool	check_if_ope(User *user) const;
		bool	check_if_user(User* user) const;
		bool	check_if_inv(std::string const username) const;
		bool	check_if_empty() const;
		bool	check_if_space_available();

		// Methods
		bool	addUser(User *user);
		bool	addOpe(User *user);
		bool	addInv(std::string const username);
		void	unset_flagInvite();
		void	unset_flagPassword();
		void	sendMessage(const std::string& message, type_sock sender_socket);
		int		user_counter();

		// Cleaners
		void	clear_topic();
		bool	removeUser(User *user);
		bool	removeOpe(User *user);
		bool	removeInv(std::string const username);

};

#endif
