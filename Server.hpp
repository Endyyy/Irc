#ifndef SERVER_HPP
# define SERVER_HPP

# include "tools.hpp"
# include "User.hpp"
# include "Channel.hpp"

# include <algorithm>
# include <arpa/inet.h>
# include <csignal>
# include <cstring>
# include <iostream>
# include <map>
# include <sstream>
# include <vector>

# define MAX_CLIENTS 100
# define BUFFER_SIZE 1024
# define NOT_SET -1

typedef int type_sock;

class Server
{
	private:
		int const			_port;
		static bool			_active;
		std::string const	_serverPassword;
		type_sock const		_serverSocket;
		sockaddr_in			_address;
		fd_set				_readfds;
		type_sock			_topSocket;

		std::map<type_sock, User*>		_clients;
		std::map<std::string, Channel*>	_channels;

		std::vector<type_sock>		_death_note;
		std::vector<std::string>	_closing_list;

		Server();
		Server(Server const& source);
		Server&	operator=(Server const& source);

	public:
		Server(type_sock port, std::string serverPassword);
		~Server();

		// Setters
		void	set_address();

		// Getters
		std::string const	get_serverPassword() const;

		// Methods
		void						bind_socket_to_address();
		void						start_listening();
		void						run();
		static void					ctrlC_behaviour(int signal);
		void						waiting_for_activity();
		type_sock					get_incoming_socket();
		void						add_new_user(type_sock userSocket);
		bool						recv_from_user(type_sock userSocket);
		void						add_empty_channels_to_closing_list();
		std::string 				get_clientDatas(type_sock socket);
		void						ask_for_login_credentials(std::string arg, type_sock client_socket, int lvl);
		int							findSocketFromNickname(std::string target);
		void						checkCommand(std::string arg, type_sock client_socket);
		void						manageInputForSic(std::string input, type_sock client_socket);
		std::vector<std::string> 	splitString(std::string input);
		bool						IsSicInput(std::string input);

		//Checkers
		bool	check_activity(type_sock socket);
		bool	rights_on_channel_name(type_sock client_socket, std::string channel_name);
		bool	check_fobidden_char_free(std::string name);

		//Cleaners
		void	reset_fd_set();
		void	erase_one_user(type_sock userSocket);
		void	erase_death_note();
		void	erase_all_users();
		void	erase_one_channel(std::string channel_name);
		void	erase_closing_list();
		void	erase_all_channels();
		void	manhunt(User* user);

		// Commands
		bool	cmdPass(std::string arg);
		bool	cmdNick(std::string arg, type_sock client_socket);
		bool	cmdUser(std::string arg, type_sock client_socket);
		void	cmdJoin(std::string arg, type_sock client_socket);
		void	cmdKick(std::string arg, type_sock client_socket);
		void	cmdPart(std::string arg, type_sock client_socket);
		void	cmdInvite(std::string arg, type_sock client_socket);
		void	cmdTopic(std::string arg, type_sock client_socket);
		void	cmdPrivMsg(std::string arg, type_sock client_socket);
		void	cmdMode(std::string arg, type_sock client_socket);

		// CmdMode tools
		void	limitManager(char mode, std::string channel_name, std::string limit);
		void	operatorManager(char mode, type_sock requester, std::string channel_name, std::string nickname);
		void	keyManager(char mode, std::string channel_name, std::string password);
		void	inviteManager(char mode, std::string channel_name);
		void	topicManager(char mode, std::string channel_name);
		int		join_with_key(type_sock client_socket, std::string channel_name, std::string key);
		int		join_without_key(type_sock client_socket, std::string channel_name);

		//ERROR_MSGS

		class ERR_ACCEPTFAILURE :		public std::exception { virtual const char* what() const throw(); };
		class ERR_BINDFAILURE :			public std::exception { virtual const char* what() const throw(); };
		class ERR_INVALIDSOCKET :		public std::exception { virtual const char* what() const throw(); };
		class ERR_LISTENINGFAILURE :	public std::exception { virtual const char* what() const throw(); };
		class ERR_SELECTFAILURE :		public std::exception { virtual const char* what() const throw(); };

};

#endif
