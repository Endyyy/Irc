#include "Server.hpp"

bool Server::_active = true;

////////////////////////////////////////////////////////////////////////////////
//  Forbidden :
Server::Server() : _port(), _serverPassword(), _serverSocket() {}
Server::Server(Server const& source) : _port(), _serverPassword(), _serverSocket() { (void)source; }
Server& Server::operator=(Server const& source) { (void)source; return (*this); }
////////////////////////////////////////////////////////////////////////////////

Server::Server(type_sock port, std::string serverPassword) :
_port(port), _serverPassword(serverPassword), _serverSocket(socket(AF_INET, SOCK_STREAM, 0))
{
	if (_serverSocket == -1)
		throw (ERR_INVALIDSOCKET());

	set_address();
	std::cout << "Server created" << std::endl;
}

Server::~Server()
{
	erase_all_channels();
	std::cout << "All channels are deleted" << std::endl;
	erase_all_users();
	std::cout << "All users are deleted" << std::endl;
	close(_serverSocket);
	std::cout << "Server destroyed" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// Setters

void	Server::set_address()
{
	//Set adress for IPV4
	_address.sin_family = AF_INET;
	// Configure socket to bind to any available IP address on the system
	_address.sin_addr.s_addr = INADDR_ANY;
	// Use our desired port number
	_address.sin_port = htons(_port);
}

////////////////////////////////////////////////////////////////////////////////
// Getters

std::string const	Server::get_serverPassword() const
{
	return (_serverPassword);
}

////////////////////////////////////////////////////////////////////////////////
// Methods

void	Server::bind_socket_to_address()
{
	if (bind(_serverSocket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
		throw (ERR_BINDFAILURE());
}

void	Server::start_listening()
{
	if (listen(_serverSocket, 3) < 0)
		throw (ERR_LISTENINGFAILURE());
	std::cout << "Server started. Waiting for connections..." << std::endl;
}

void	Server::run()
{
	type_sock socket = 0;

	signal(SIGINT, ctrlC_behaviour);

	while (_active)
	{
		std::cout << std::endl << "main loop" << std::endl;

		reset_fd_set();
		std::cout << "fd_set reseted" << std::endl;

		waiting_for_activity();
		std::cout << "waiting is over" << std::endl;

		// Server part

		if (_active && check_activity(_serverSocket))
		{
			std::cout << "connection attempt detected on server" << std::endl;

			socket = get_incoming_socket();
			std::cout << "socket identified" << std::endl;

			if (_active && _clients.find(socket) == _clients.end())
			{
				std::cout << "user not find in map" << std::endl;

				add_new_user(socket);
				std::cout << "new user added" << std::endl;

				if (_active && _clients.find(socket) != _clients.end() && _clients[socket]->get_userState() == 0)
					send(socket, "PASS <password>\n", strlen("PASS <password>\n"), 0);
			}
		}

		// Client part

		for (std::map<type_sock, User*>::iterator it = _clients.begin(); _active && it != _clients.end(); it++)
		{
			std::cout << "clients loop" << std::endl;
			socket = it->first;
			if (_active && check_activity(it->second->get_userSocket()))
			{
				std::cout << "client activity detected" << std::endl;
				try
				{
					if (_active)
					{
						if (recv_from_user(socket))
						{
							std::string	input = _clients[socket]->get_inputStack();
							std::cout << "input correctly recieved" << std::endl;

							checkCommand(input, socket);
							std::cout << "command checked" << std::endl;
							if (_active && _clients.find(socket) != _clients.end())
								_clients[socket]->reset_inputStack();
						}
					}
				}
				catch(std::exception const& e)
				{
					if (_active)
					{
						std::string str = e.what();
						if (str == "client disconnected")
							std::cout << "Client disconnected. Info : " << get_clientDatas(socket) << std::endl;
						else if (str == "recv error")
							std::cout << "Lost connection with client. Info : " << get_clientDatas(socket) << std::endl;
						_death_note.push_back(socket);
					}
				}
			}
		}

		if (_active)
		{
			std::cout << "cleaning iteration" << std::endl;
			erase_death_note();
			add_empty_channels_to_closing_list();
			erase_closing_list();
		}
	}
}

void	Server::ctrlC_behaviour(int signal)
{
	(void)signal;
	_active = false;
	// std::cout << "CTRL C SUCCESS" << std::endl;
}

void	Server::waiting_for_activity()
{
	std::cout << "before select" << std::endl;
	int activity = select(_topSocket + 1, &_readfds, NULL, NULL, NULL);
	std::cout << "after select" << std::endl;
	if (_active && activity < 0)
		throw (ERR_SELECTFAILURE());
}

type_sock	Server::get_incoming_socket()
{
	socklen_t addrLength = sizeof(_address);
	type_sock tmp_socket = accept(_serverSocket, (struct sockaddr *)&_address, &addrLength);
	if (tmp_socket < 0)
		throw (ERR_ACCEPTFAILURE());
	return (tmp_socket);
}

void	Server::add_new_user(type_sock userSocket)
{
	_clients.insert(std::make_pair(userSocket, new User(userSocket)));
	std::cout <<
	"New connection, socket fd: " << userSocket <<
	", IP: " << inet_ntoa(_address.sin_addr) <<
	", Port: " << ntohs(_address.sin_port) <<
	std::endl;
}

bool	Server::recv_from_user(type_sock userSocket)
{
	char		buffer[BUFFER_SIZE] = {0};
	ssize_t bytes = recv(userSocket, buffer, BUFFER_SIZE, MSG_DONTWAIT);
	std::cout << "input received. bytes = " << bytes << std::endl;

	if (bytes < 0)
	{
		std::cout << "except Problem with recv, bytes == -1" << std::endl;
		throw (std::runtime_error("recv error"));
	}
	if (bytes == 0)
	{
		std::cout << "except disconnection detected, bytes == 0" << std::endl;
		throw (std::runtime_error("client disconnected"));
	}
	if (bytes > 0)
	{
		std::cout << "normal behaviour, bytes > 0" << std::endl;
		_clients[userSocket]->add_to_inputStack(buffer);
		if (_clients[userSocket]->get_inputStack()[_clients[userSocket]->get_inputStack().size() - 1] == '\n')
		{
			_clients[userSocket]->set_inputStack(_clients[userSocket]->get_inputStack().erase(_clients[userSocket]->get_inputStack().size() - 1));
			std::cout << "input cleaned" << std::endl;
			return (true);
		}
		else
			send(userSocket, "^D", strlen("^D"), 0);
	}
	return (false);
}

void	Server::add_empty_channels_to_closing_list()
{
	for (std::map<std::string, Channel*>::iterator chan_it = _channels.begin(); chan_it != _channels.end(); chan_it++)
	{
		if (chan_it->second->check_if_empty())
		{
			_closing_list.push_back(chan_it->first);
			std::cout << "channel is pushed back on closing list" << std::endl;
		}
	}
}

std::string Server::get_clientDatas(type_sock socket)
{
	std::string str = "";
	std::string tmp = _clients[socket]->get_nickname();
	std::ostringstream oss;
	oss << socket;
	if (tmp != "")
		str += "Nickname \'" + tmp + "\'; ";
	tmp = _clients[socket]->get_username();
	if (tmp != "")
		str += "Username \'" + tmp + "\'; ";
	str += "Socket " + oss.str() + ";";
	return (str);
}

std::vector<std::string> Server::splitString(std::string input)
{
    std::vector<std::string> tokens;

    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, '\n'))
        tokens.push_back(token);
    return tokens;
}

void Server::manageInputForSic(std::string input, type_sock client_socket)
{
	std::string user;
	std::vector<std::string> tokens;

	if (input.substr(0,4) != "PASS") //PASS lol\r\nNICK yo\r\nUSER yo localhost 127.0.0.1 :yo\r
	{
		send(client_socket, "Please relaunch using : ./sic -h 127.0.0.1 -p [port] -k [pass] -n [nickname]\n", \
		strlen("Please relaunch using : ./sic -h 127.0.0.1 -p [port] -k [pass] -n [nickname]\n"), 0);
		return ;
	}
	else
	{
		input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
		tokens = splitString(input);
		std::stringstream stream(tokens[2]);
		std::string tmp;
		stream >> user;
		user += " ";
		stream >> tmp;
		user += tmp;
	}
	if (cmdPass(tokens[0]) && cmdNick(tokens[1], client_socket) && cmdUser(user, client_socket))
	{
		std::cout << "Registration succesful" << std::endl;
		send(client_socket, "Password granted !\nYou are now one of us !\n", strlen("Password granted !\nYou are now one of us !\n"), 0);
	}
	else
	{
		std::cout << "tokens[0] : " << tokens[0] << std::endl;
		std::cout << "tokens[1] : " << tokens[1] << std::endl;
		std::cout << "user : " << user << std::endl;
		send(client_socket, "Please relaunch using : ./sic -h 127.0.0.1 -p [port] -k [pass] -n [nickname]\n", \
		strlen("Please relaunch using : ./sic -h 127.0.0.1 -p [port] -k [pass] -n [nickname]\n"), 0);
		return ;
	}
	_clients[client_socket]->set_userState(3);
}

void	Server::ask_for_login_credentials(std::string input, type_sock client_socket, int lvl)
{
	std::cout << "input : " << input << std::endl;
	if (lvl == 0)
	{
		std::cout << "user is lvl 0" << std::endl;
		if (cmdPass(input))
		{
			_clients[client_socket]->set_userState(1);
			std::cout << "user has become lvl 1" << std::endl;
			send(client_socket, "NICK <nickname>\n", strlen("NICK <nickname>\n"), 0);
		}
		else
			send(client_socket, "PASS <password>\n", strlen("PASS <password>\n"), 0);
			//ask for password again
	}
	else if (lvl == 1)
	{
		std::cout << "user is lvl 1" << std::endl;

		if (cmdNick(input, client_socket))
		{
			_clients[client_socket]->set_userState(2);
			std::cout << "user has become lvl 2" << std::endl;
			send(client_socket, "USER <username>\n", strlen("USER <username>\n"), 0);
		}
		else
			send(client_socket, "NICK <nickname>\n", strlen("NICK <nickname>\n"), 0);
			//ask for another nickname
	}
	else if (lvl == 2)
	{
		std::cout << "user is lvl 2" << std::endl;

		if (cmdUser(input, client_socket))
		{
			_clients[client_socket]->set_userState(3);
			std::cout << "user has become lvl 3" << std::endl;
			send(client_socket, "You are now one of us !\n", strlen("You are now one of us !\n"), 0);
		}
		else
			send(client_socket, "USER <username>\n", strlen("USER <username>\n"), 0);
			//ask for a valid username
	}
}

type_sock	Server::findSocketFromNickname(std::string target)
{
	type_sock targetSocket = -1;
	for (std::map<type_sock, User*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		User* user = it->second;
		if (user->get_nickname() == target)
		{
			targetSocket = user->get_userSocket();
			break ;
		}
	}
	return (targetSocket);
}

bool	Server::IsSicInput(std::string input)
{
	for (int i = 0; input[i]; i++)
	{
		if (input[i] == '\r')
			return(true);
	}
	return (false);
}

void Server::checkCommand(std::string input, type_sock client_socket)
{
	int					lvl = _clients[client_socket]->get_userState();
	std::stringstream	stream(input);
	std::string			cmd;

	stream >> cmd;
	if (cmd == "QUIT" && (input.size() == 4 || input.size() == 5))
		_death_note.push_back(client_socket);
	else if (lvl < 3)
	{
		if (IsSicInput(input))
			manageInputForSic(input, client_socket);
		else
			ask_for_login_credentials(input, client_socket, lvl);
	}
	else if (cmd.size())
	{
		if (cmd == "JOIN")
			cmdJoin(input, client_socket);
		else if (cmd == "PRIVMSG")
			cmdPrivMsg(input, client_socket);
		else if (cmd == "PART")
			cmdPart(input, client_socket);
		else if (cmd == "INVITE")
			cmdInvite(input, client_socket);
		else if (cmd == "KICK")
			cmdKick(input, client_socket);
		else if (cmd == "MODE")
			cmdMode(input, client_socket);
		else if (cmd == "TOPIC")
			cmdTopic(input, client_socket);
		else if (cmd == "NICK")
			cmdNick(input, client_socket);
		else
			send(client_socket, "Commands available : JOIN, PRIVMSG, INVITE, KICK, MODE, TOPIC, NICK, PART, QUIT.\n",\
			strlen("Commands available : JOIN, PRIVMSG, INVITE, KICK, MODE, TOPIC, NICK, PART, QUIT.\n"), 0);
	}
	std::cout << "Received data from client, socket fd: " << client_socket << ", Data: " << input << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
//Checkers

bool	Server::check_activity(type_sock socket)
{
	return (FD_ISSET(socket, &_readfds));
}

bool	Server::rights_on_channel_name(type_sock client_socket, std::string channel_name)
{
	std::map<std::string, Channel*>::iterator chan_it = _channels.find(channel_name);
	std::map<type_sock, User*>::iterator client_it = _clients.find(client_socket);
	if (chan_it != _channels.end() && client_it != _clients.end())
		if (chan_it->second->check_if_ope(client_it->second))
			return (true);
	return (false);
}

bool	Server::check_fobidden_char_free(std::string name)
{
	for (int i = 0; name[i]; i++)
	{
		if (name[i] <= 32 || name[i] == 127 || name[i] == ':'
		|| name[i] == ',' || name[i] == ';' || name[i] == '!' || name[i] == '#')
			return (false);
	}
	return (true);
}

////////////////////////////////////////////////////////////////////////////////
//Cleaners

void	Server::reset_fd_set()
{
	// Erase all preexisting socket values
	FD_ZERO(&_readfds);

	// Add the server socket to the set
	FD_SET(_serverSocket, &_readfds);
	_topSocket = _serverSocket;

	// Add clients sockets to the set
	for (std::map<int, User*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->first > 0)
			FD_SET(it->first, &_readfds);
		if (it->first > _topSocket)
			_topSocket = it->first;
	}
}

void	Server::erase_one_user(type_sock userSocket)
{
	std::map<type_sock, User*>::iterator it = _clients.find(userSocket);
	if (it != _clients.end())
	{
		manhunt(it->second);
		send(userSocket, "Press 'Enter' to leave\n", strlen("Press 'Enter' to leave\n"), 0);
		FD_CLR(userSocket, &_readfds);
		close(userSocket);
		delete it->second;
		it->second = NULL;
		_clients.erase(it);
	}
}

void	Server::erase_death_note()
{
	for (std::vector<type_sock>::iterator it = _death_note.begin(); it != _death_note.end(); it = _death_note.begin())
	{
		erase_one_user(*it);
		std::cout << "user erased from map" << std::endl;
		_death_note.erase(it);
		std::cout << "user erased from vector" << std::endl;
	}
}

void	Server::erase_all_users()
{
	for (std::map<type_sock, User*>::iterator it = _clients.begin(); it != _clients.end(); it++)
		_death_note.push_back(it->first);

	erase_death_note();
}

void	Server::erase_one_channel(std::string channel_name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(channel_name);
	if (it != _channels.end())
	{
		delete it->second;
		it->second = NULL;
		_channels.erase(it);
	}
}

void	Server::erase_closing_list()
{
	for (std::vector<std::string>::iterator it = _closing_list.begin(); it != _closing_list.end(); it = _closing_list.begin())
	{
		erase_one_channel(*it);
		std::cout << "channel erased from map" << std::endl;
		_closing_list.erase(it);
		std::cout << "channel erased from vector" << std::endl;
	}
}

void	Server::erase_all_channels()
{
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); it++)
		_closing_list.push_back(it->first);

	erase_closing_list();
}

void	Server::manhunt(User* user)
{
	for (std::map<std::string, Channel*>::iterator chan_it = _channels.begin(); chan_it != _channels.end(); chan_it++)
	{
		if (chan_it->second->check_if_user(user))
			chan_it->second->removeUser(user);
	}
	std::cout << "manhunt done" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// Commands

bool Server::cmdPass(std::string input)
{
	std::stringstream	stream(input);
	std::string			cmd;
	std::string			password;
	std::string			end;

	std::cout << "cmdPass" << std::endl;

	stream >> cmd;
	stream >> password;
	stream >> end;

	if (cmd.size() == 0 || password.size() == 0 || end.size() != 0 || cmd != "PASS")
		return (false);
	if (password == get_serverPassword())
		return (true);
	return (false);
}

bool Server::cmdNick(std::string input, type_sock client_socket)
{
	std::stringstream	stream(input);
	std::string			cmd;
	std::string			nickname;
	std::string			end;

	stream >> cmd;
	stream >> nickname;
	stream >> end;

	std::cout << "cmdNick" << std::endl;

	if (cmd.size() == 0 || nickname.size() == 0 || end.size() != 0 || cmd != ("NICK"))
		return (false);

	// forbiden character test
	if (!check_fobidden_char_free(nickname))
	{
		send(client_socket, "Abort. Nickname cannot contain whitespaces or : , ; ! or #\n",
		strlen("Abort. Nickname cannot contain whitespaces or : , ; ! or #\n"), 0);
		return (false);
	}

	// availability test
	for (std::map<type_sock, User*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->get_nickname() == nickname && nickname[0] != '\n')
		{
			send(client_socket, "Nickname already taken.\n", strlen("Nickname already taken.\n"), 0);
			return (false);
		}
	}

	_clients[client_socket]->set_nickname(nickname);
	return (true);
}

bool Server::cmdUser(std::string input, type_sock client_socket)
{
	std::stringstream	stream(input);
	std::string			cmd;
	std::string			username;
	std::string			end;

	std::cout << "cmdUser" << std::endl;

	stream >> cmd;
	stream >> username;
	stream >> end;

	if (cmd.size() == 0 || username.size() < 1 || end.size() != 0 || cmd != "USER")
		return (false);

	username.erase(username.begin());
	if (!check_fobidden_char_free(username))
	{
			send(client_socket, "Abort. Username cannot contain whitespaces or : , ; ! or #\n",
			strlen("Abort. Username cannot contain whitespaces or : , ; ! or #\n"), 0);
			return (false);
	}

	_clients[client_socket]->set_username(username);
	return (true);
}

void Server::cmdJoin(std::string input, type_sock client_socket)
{
	std::stringstream	stream(input);
	std::string			cmd;
	std::string			channel_name;
	std::string			key;
	std::string			end;
	int					success = 0;

	stream >> cmd;
	stream >> channel_name;
	stream >> key;
	stream >> end;

	if (cmd.size() == 0 || channel_name.size() < 2 || channel_name[0] != '#' || end.size() != 0)
	{
		send(client_socket, "Usage : JOIN <#channel_name> <(key)>\n", strlen("Usage : JOIN <#channel_name> <(key)>\n"), 0);
		return ;
	}

	std::map<std::string, Channel*>::iterator chan_it = _channels.find(channel_name);
	std::map<type_sock, User*>::iterator client_it = _clients.find(client_socket);

	if (key.size())
		success = join_with_key(client_socket, channel_name, key);
	else
		success = join_without_key(client_socket, channel_name);

	if (success > 1) // channel creation
	{
		_channels.insert(std::make_pair(channel_name, new Channel(channel_name, client_it->second)));
		send(client_it->second->get_userSocket(), "You have become member and operator of the channel !\n", strlen("You have become member and operator of the channel !\n"), 0);
		std::cout << "channel creation done in cmdJoin" << std::endl;
	}
	else if (success) // channel already existing
	{
		if (chan_it->second->addUser(client_it->second))
		{
			send(client_it->second->get_userSocket(), "You have become member of the channel !\n", strlen("You have become member of the channel !\n"), 0);
			std::cout << "add user (if not existing already) done in cmdJoin" << std::endl;
			if (chan_it != _channels.end())
			{
				std::string user_join_message = ":" + client_it->second->get_nickname() + " JOIN " + channel_name + "\n";
				chan_it->second->sendMessage(user_join_message, client_socket);
			}
		}
	}
}

void Server::cmdKick(std::string arg, type_sock client_socket)
{
	std::stringstream	stream(arg);
	std::string			cmd;
	std::string			channel_name;
	std::string			banned_user;
	std::string			ban_msg;

	if (!(stream >> cmd) || cmd != "KICK") //Check commande
		return ;
	if (!(stream >> channel_name))		 //Check nom du channel
	{
		send(client_socket, "KICK <#channel_name> <nickname> [<comment>]\n", strlen("KICK <#channel_name> <nickname> [<comment>]\n"), 0);
		return ;
	}
	if (channel_name[0] != '#' || channel_name.size() == 1) //Check syntaxe du channel
	{
		send(client_socket, "KICK <#channel_name> <nickname> [<comment>]\n", strlen("KICK <#channel_name> <nickname> [<comment>]\n"), 0);
		return ;
	}
	if (!(stream >> banned_user))						   //Check target
	{
		send(client_socket, "KICK <#channel_name> <nickname> [<comment>]\n", strlen("KICK <#channel_name> <nickname> [<comment>]\n"), 0);
		return ;
	}
	if (stream)											//Recupere message s'il y'en a un
	{
		stream >> std::ws;
		std::getline(stream, ban_msg);
	}
	if (!(_channels[channel_name]->check_if_ope(_clients[client_socket]))) //Check si operateur ou non
	{
		send(client_socket, "You don't have the right to use this command !\n", \
		strlen("You don't have the right to use this command !\n"), 0);
		return ;
	}
	if (_channels.find(channel_name) == _channels.end())				  //Check si channel existe
	{
		send(client_socket, "This channel does not exist !\n", strlen("This channel does not exist !\n"), 0);
		return ;
	}
	type_sock targetSocket = findSocketFromNickname(banned_user);
	if (targetSocket == -1)											   //Check si target existe
	{
		send(client_socket, "Target does not exist !\n", strlen("Target does not exist !\n"), 0);
		return ;
	}
	if (!(_channels[channel_name]->check_if_user(_clients[targetSocket])) && !(_channels[channel_name]->check_if_ope(_clients[targetSocket]))) //Check si target a kick registered sur channel
	{
		send(client_socket, "The target is not registered in the channel !\n", strlen("The target is not registered in the channel !\n"), 0);
		return ;
	}
	if (banned_user == _clients[client_socket]->get_nickname())  //Check si client essaie de se kick
	{
		send(client_socket, "Use PART <#channel_name> to leave a channel , don't KICK yourself bro\n", strlen("Use PART <#channel_name> to leave a channel , don't KICK yourself bro\n"), 0);
		return ;
	}
	_channels[channel_name]->removeUser(_clients[targetSocket]);
	if (!ban_msg.empty())										//Check si pas de message specifique
	{
		std::string ban_message_plus = channel_name + " :You were kicked by " + _clients[client_socket]->get_nickname() + " (" + ban_msg + ")\n";
		send(targetSocket, ban_message_plus.c_str(), ban_message_plus.size(), 0);
		std::string ban_chan_message_plus = channel_name + " :" + _clients[targetSocket]->get_nickname() + " was kicked by " + _clients[client_socket]->get_nickname() + " (" + ban_msg + ")\n";
		_channels[channel_name]->sendMessage(ban_chan_message_plus, client_socket);
	}
	else														 //Envoi le message specifique
	{
		std::string ban_message = channel_name + " :You were kicked by " + _clients[client_socket]->get_nickname() + "\n";
		send(targetSocket, ban_message.c_str(), ban_message.size(), 0);
		std::string ban_chan_message = channel_name + " :" + _clients[targetSocket]->get_nickname() + " was kicked by " + _clients[client_socket]->get_nickname() + "\n";
		_channels[channel_name]->sendMessage(ban_chan_message, client_socket);
	}
}

void Server::cmdPart(std::string arg, type_sock client_socket)
{
	std::stringstream	stream(arg);
	std::string			cmd;
	std::string			channel_name;
	std::string			end;

	if (!(stream >> cmd) || cmd != "PART") //Check commande
		return ;
	if (!(stream >> channel_name))		//Check nom du channel
	{
		send(client_socket, "PART <#channel_name>\n", strlen("PART <#channel_name>\n"), 0);
		return ;
	}
	if (channel_name[0] != '#' || channel_name.size() == 1) //Check syntaxe nom du channel
	{
		send(client_socket, "PART <#channel_name>\n", strlen("PART <#channel_name>\n"), 0);
		return ;
	}
	if (stream) //Check si argument superflues
	{
		stream >> end;
		if (!end.empty())
		{
			send(client_socket, "PART <#channel_name>\n", strlen("PART <#channel_name>\n"), 0);
			return ;
		}
	}
	if (_channels.find(channel_name) == _channels.end()) //Check si channel existe
	{
		send(client_socket, "This channel does not exist !\n", strlen("This channel does not exist !\n"), 0);
		return ;
	}
	if (!(_channels[channel_name]->check_if_user(_clients[client_socket])) && !(_channels[channel_name]->check_if_ope(_clients[client_socket]))) //Check si registered au channel
	{
		send(client_socket, "You're not even registered in the channel !\n", strlen("You're not even registered in the channel !\n"), 0);
		return ;
	}
	_channels[channel_name]->removeUser(_clients[client_socket]);
	std::string leaving_message = ":" + _clients[client_socket]->get_nickname() + " has left " + channel_name + "\n";
	_channels[channel_name]->sendMessage(leaving_message, client_socket);
	send(client_socket, "You left the channel !\n", strlen("You left the channel !\n"), 0);
}

void Server::cmdInvite(std::string arg, type_sock client_socket)
{
	std::stringstream	stream(arg);
	std::string			cmd;
	std::string			channel_name;
	std::string			invited_user;
	std::string			end;

	if (!(stream >> cmd) || cmd != "INVITE")  //Check commande
		return ;
	if (!(stream >> channel_name))			//Check nom du channel
	{
		send(client_socket, "INVITE <#channel_name> <nickname>\n", strlen("INVITE <#channel_name> <nickname>\n"), 0);
		return ;
	}
	if (channel_name[0] != '#' || channel_name.size() == 1)  //Check syntaxe nom du channel
	{
		send(client_socket, "INVITE <#channel_name> <nickname>\n", strlen("INVITE <#channel_name> <nickname>\n"), 0);
		return ;
	}
	if (!(stream >> invited_user))						   //Check 3eme arg
	{
		send(client_socket, "INVITE <#channel_name> <nickname>\n", strlen("INVITE <#channel_name> <nickname>\n"), 0);
		return ;
	}
	if (stream) 											//Check si argument superflues
	{
		stream >> end;
		if (!end.empty())
		{
			send(client_socket, "INVITE <#channel_name> <nickname>\n", strlen("INVITE <#channel_name> <nickname>\n"), 0);
			return ;
		}
	}
	if (_channels.find(channel_name) == _channels.end())	 //Check si channel existe
	{
		send(client_socket, "This channel does not exist !\n", strlen("This channel does not exist !\n"), 0);
		return ;
	}
	type_sock targetSocket = findSocketFromNickname(invited_user);
	if (targetSocket == -1)								  //Check si la target existe
	{
		send(client_socket, "Target does not exist !\n", strlen("Target does not exist !\n"), 0);
		return ;
	}
	if (_channels[channel_name]->check_if_user(_clients[targetSocket]) || _channels[channel_name]->check_if_ope(_clients[targetSocket])) //Check si target déja dans channel
	{
		send(client_socket, "The target is already registered in the channel !\n", strlen("The target is already registered in the channel !\n"), 0);
		return ;
	}
	if (!(_channels[channel_name]->check_if_ope(_clients[client_socket]))) //Check si le droit d'invité ou non
	{
		send(client_socket, "You don't have the right to use this command !\n",
		strlen("You don't have the right to use this command !\n"), 0);
		return ;
	}
	if (_channels[channel_name]->addUser(_clients[targetSocket]))
	{
		std::string invite_message = "You were invited by " + _clients[client_socket]->get_nickname() + " on " + channel_name + "\n";
		send(targetSocket ,invite_message.c_str(), invite_message.size(), 0);
	}
}

void Server::cmdTopic(std::string arg, type_sock client_socket)
{
	std::stringstream	stream(arg);
	std::string			cmd;
	std::string			channel_name;
	std::string			topic;

	if (!(stream >> cmd) || cmd != "TOPIC") 	//Check nom de la commande
		return ;
	if (!(stream >> channel_name))			  //Check nom du channel
	{
		send(client_socket, "TOPIC <#channel_name> :<topic>\n", strlen("TOPIC <#channel_name> :<topic>\n"), 0);
		return ;
	}
	if (channel_name[0] != '#' || channel_name.size() == 1) //Check syntaxe du nom du channel
	{
		send(client_socket, "TOPIC <#channel_name> :<topic>\n", strlen("TOPIC <#channel_name> :<topic>\n"), 0);
		return ;
	}
	if (stream)												//Recupere potentiel 3eme argument
	{
		stream >> std::ws;
		std::getline(stream, topic);
	}
	if (_channels.find(channel_name) == _channels.end())   //Check si channel existe
	{
		send(client_socket, "This channel does not exist !\n", strlen("This channel does not exist !\n"), 0);
		return ;
	}
	if (!(_channels[channel_name]->check_if_user(_clients[client_socket])) && !(_channels[channel_name]->check_if_ope(_clients[client_socket]))) //Check si user dans channel
	{
		send(client_socket, "You're not registered in the channel !\n", strlen("You're not registered in the channel !\n"), 0);
		return ;
	}
	if (topic.empty()) //Check si argument topic donné, si non affiche topic
	{
		if (_channels[channel_name]->get_topic().empty())
		{
			send(client_socket, "There is no active topic on this channel !\n", strlen("There is no active topic on this channel !\n"), 0);
			return ;
		}
		std::string topic_message = "TOPIC " + channel_name + " " + _channels[channel_name]->get_topic() + "\n";
		send(client_socket, topic_message.c_str(), topic_message.size(), 0);
	}
	else
	{
		if (topic[0] != ':') //Check syntaxe si topic donné
		{
			send(client_socket, "TOPIC <#channel_name> :<topic>\n", strlen("TOPIC <#channel_name> :<topic>\n"), 0);
			return ;
		}
		if (_channels[channel_name]->get_flagTopic() && !(_channels[channel_name]->check_if_ope(_clients[client_socket]))) //Check si flag topic on et si opérateur
		{
			send(client_socket, "You don't have the right to use this command !\n",
			strlen("You don't have the right to use this command !\n"), 0);
			return ;
		}
		if (topic.size() == 1) //Check si size 1 et clear topic
		{
			_channels[channel_name]->clear_topic();
			return ;
		}
		_channels[channel_name]->set_topic(topic);
		std::string new_topic_message = "TOPIC " + channel_name + " " + _channels[channel_name]->get_topic() + "\n";
		_channels[channel_name]->sendMessage(new_topic_message, client_socket);
		return ;
	}
}

void Server::cmdPrivMsg(std::string arg, type_sock client_socket)
{
	std::stringstream	stream(arg);
	std::string			cmd;
	std::string			target;
	std::string			message;

	if (!(stream >> cmd) || cmd != "PRIVMSG") 	//Check nom de la commande
		return ;
	if (!(stream >> target))					//Check s'il y'a une target
	{
		send(client_socket, "PRIVMSG <target> :<message>\n", strlen("PRIVMSG <target> :<message>\n"), 0);
		return ;
	}
	if (stream)									//Recupere potentiel 3eme argument
	{
		stream >> std::ws;
		std::getline(stream, message);
	}
	if (message.empty() || message[0] != ':') //Check syntaxe message
	{
		send(client_socket, "PRIVMSG <target> :<message>\n", strlen("PRIVMSG <target> :<message>\n"), 0);
		return ;
	}
	if (target[0] == '#')					//Check si la target est un channel
	{
		if (target.size() == 1 || _channels.find(target) == _channels.end())  //Check si channel existe
		{
			send(client_socket, "This channel does not exist !\n", strlen("This channel does not exist !\n"), 0);
			return ;
		}
		if (!(_channels[target]->check_if_user(_clients[client_socket])) && !(_channels[target]->check_if_ope(_clients[client_socket])))	//Check si sur le channel
		{
			send(client_socket, "You're not registered in the channel !\n", strlen("You're not registered in the channel !\n"), 0);
			return ;
		}
		std::string priv_message_chan = target + " <" + _clients[client_socket]->get_nickname() + "> " + message + "\n";
		_channels[target]->sendMessage(priv_message_chan, client_socket);
	}
	else																							//Sinon message vers un user
	{
		std::string priv_message = "<" + _clients[client_socket]->get_nickname() + "> " + message + "\n";
		type_sock targetSocket = findSocketFromNickname(target);
		if (targetSocket == -1)
		{
			send(client_socket, "Target does not exist !\n", strlen("Target does not exist !\n"), 0);
			return ;
		}
		send(targetSocket, priv_message.c_str(), priv_message.size(), 0);
	}

}

void Server::cmdMode(std::string arg, type_sock client_socket)
{
	std::stringstream	stream(arg);
	std::string			cmd;
	std::string			channel_name;
	std::string			modes;
	std::string			parameter;
	std::string			end;

	stream >> cmd;
	stream >> channel_name;
	stream >> modes;
	stream >> parameter;
	stream >> end;

	if (cmd.size() == 0 || channel_name.size() == 0 || modes.size() == 0
	|| modes.size() > 2 || end.size() != 0) // size check
		send(client_socket, "Usage : MODE <channel> {[+|-]l|o|k|i|t} [<parameter>]\n", strlen("Usage : MODE <channel> {[+|-]l|o|k|i|t} [<parameter>]\n"), 0);
	else if (channel_name[0] != '#' || (modes[0] != '-' && modes[0] != '+')
	|| (modes[1] != 'o' && modes[1] != 'i' && modes[1] != 't' && modes[1] != 'k' && modes[1] != 'l'))//syntax check
		send(client_socket, "Usage : MODE <channel> {[+|-]l|o|k|i|t} [<parameter>]\n", strlen("Usage : MODE <channel> {[+|-]l|o|k|i|t} [<parameter>]\n"), 0);
	else if (!rights_on_channel_name(client_socket, channel_name))
		send(client_socket, "You have no operator rights on this channel\n", strlen("You have no operator rights on this channel\n"), 0);
	else
	{
		if (parameter.size() != 0)
		{
			if (modes[1] == 'l')
				limitManager(modes[0], channel_name, parameter);
			else if (modes[1] == 'o')
				operatorManager(modes[0], client_socket, channel_name, parameter);
			else if (modes[1] == 'k')
				keyManager(modes[0], channel_name, parameter);
			else
				send(client_socket, "Usage : MODE <channel> {[+|-]l|o|k|i|t} [<parameter>]\n", strlen("Usage : MODE <channel> {[+|-]l|o|k|i|t} [<parameter>]\n"), 0);
		}
		else
		{
			if (modes[1] == 'l')
				limitManager(modes[0], channel_name, "1");
			else if (modes[1] == 'k')
				keyManager(modes[0], channel_name, "");
			else if (modes[1] == 'i')
				inviteManager(modes[0], channel_name);
			else if (modes[1] == 't')
				topicManager(modes[0], channel_name);
			else
				send(client_socket, "Usage : MODE <channel> {-]l|k|i|t}\n", strlen("Usage : MODE <channel> {-]l|k|i|t}\n"), 0);
		}
		if ((modes[1] == 't' || modes[1] == 'i') && parameter.size() != 0)
				send(client_socket, "Usage : MODE <channel> {[+|-]t|i}\n", strlen("Usage : MODE <channel> {[+|-]t|i}\n"), 0);
	}
}

////////////////////////////////////////////////////////////////////////////////
// CmdMode tools

void	Server::limitManager(char mode, std::string channel_name, std::string limit)
{
	// — l : Définir/supprimer la limite d’utilisateurs pour le canal
	std::map<std::string, Channel*>::iterator it = _channels.find(channel_name);
	if (it != _channels.end() && check_valid_limit(limit, MAX_CLIENTS))
	{
		if (mode == '+')
			it->second->setLimit(atoi(limit.c_str()));
		else
			it->second->setLimit(NOT_SET);
	}
}

void	Server::operatorManager(char mode, type_sock requester, std::string channel_name, std::string nickname)
{
	// — o : Donner/retirer le privilège de l’opérateur de canal
	std::map<std::string, Channel*>::iterator chan_it = _channels.find(channel_name);
	std::map<type_sock, User*>::iterator target_it = _clients.find(findSocketFromNickname(nickname));
	std::map<type_sock, User*>::iterator req_it = _clients.find(requester);
	if (chan_it == _channels.end() && target_it == _clients.end())
		return ;
	if (req_it != target_it && chan_it->second->check_if_user(target_it->second))
	{
		if (mode == '+')
			chan_it->second->addOpe(target_it->second);
		else
			chan_it->second->removeOpe(target_it->second);
	}
}

void	Server::keyManager(char mode, std::string channel_name, std::string password)
{
	// — k : Définir/supprimer la clé du canal (mot de passe)
	std::map<std::string, Channel*>::iterator it = _channels.find(channel_name);
	if (it != _channels.end())//checker si password valide ? caracteres interdits?
	{
		if (mode == '+')
		{
			it->second->set_flagPassword(true);
			it->second->set_password(password);
		}
		else
		{
			it->second->set_flagPassword(false);
			it->second->set_password(password);
		}
	}
}

void	Server::inviteManager(char mode, std::string channel_name)
{
	std::map<std::string, Channel*>::iterator chan_it = _channels.find(channel_name);
	if (chan_it != _channels.end())
	{
		if (mode == '+')
			chan_it->second->set_flagInvite(true);
		else
			chan_it->second->set_flagInvite(false);
	}
}

void	Server::topicManager(char mode, std::string channel_name)
{
	std::map<std::string, Channel*>::iterator chan_it = _channels.find(channel_name);
	if (chan_it != _channels.end())
	{
		if (mode == '+')
			chan_it->second->set_flagTopic(true);
		else
			chan_it->second->set_flagTopic(false);
	}
}

int	Server::join_with_key(type_sock client_socket, std::string channel_name, std::string key)
{
	std::map<std::string, Channel*>::iterator chan_it = _channels.find(channel_name);
	std::map<type_sock, User*>::iterator client_it = _clients.find(client_socket);

	if (chan_it == _channels.end())// no channel, no password
		send(client_socket, "There is no such channel. Abort.\n", strlen("There is no such channel. Abort.\n"), 0);
	else if (!chan_it->second->get_flagPassword()) // password not required
		send(client_socket, "No password required for this channel. Abort.\n", strlen("No password required for this channel. Abort.\n"), 0);
	else if (chan_it->second->get_flagInvite())// invitation required
	{
		if (!chan_it->second->check_if_inv(client_it->second->get_nickname())) // not found on invitation list
			send(client_socket, "You cannot join this channel without being invitated to. Abort.\n", strlen("You cannot join this channel without being invitated to. Abort.\n"), 0);
		else if (key != chan_it->second->get_password()) // invited but wrong password
			send(client_socket, "Wrong password. Abort.\n", strlen("Wrong password. Abort.\n"), 0);
		else // password ok, invitation ok
			return (1);
	}
	else if (key != chan_it->second->get_password()) // password required but not invitation
		send(client_socket, "Wrong password. Abort.\n", strlen("Wrong password. Abort.\n"), 0);
	else // password only ok
		return (1);
	return (0);
}

int	Server::join_without_key(type_sock client_socket, std::string channel_name)
{
	std::map<std::string, Channel*>::iterator chan_it = _channels.find(channel_name);
	std::map<type_sock, User*>::iterator client_it = _clients.find(client_socket);

	if (chan_it == _channels.end())// no channel, no password
		return (2);
	else if (chan_it->second->get_flagPassword()) // password required
		send(client_socket, "Password required for this channel. Abort.\n", strlen("Password required for this channel. Abort.\n"), 0);
	else if (chan_it->second->get_flagInvite())// invitation only required
	{
		if (!chan_it->second->check_if_inv(client_it->second->get_nickname())) // not invited
			send(client_socket, "You cannot join this channel without being invitated to. Abort.\n", strlen("You cannot join this channel without being invitated to. Abort.\n"), 0);
		else // invited
			return (1);
	}
	else
		return (1); // no password neither invitation required
	return (0);
}


////////////////////////////////////////////////////////////////////////////////
//  ERROR_MSGS
const char *Server::ERR_ACCEPTFAILURE::what() const throw()		{ return "Accept error"; }

const char *Server::ERR_BINDFAILURE::what() const throw()		{ return "Failed to bind socket to address"; }

const char *Server::ERR_INVALIDSOCKET::what() const throw()		{ return "Error creating socket"; }

const char *Server::ERR_LISTENINGFAILURE::what() const throw()	{ return "Socket failed to start listening"; }

const char *Server::ERR_SELECTFAILURE::what() const throw()		{ return "Select error"; }
////////////////////////////////////////////////////////////////////////////////
