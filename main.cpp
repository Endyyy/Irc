#include <iostream>
#include <stdexcept>
#include "tools.hpp"
#include "Server.hpp"
#include "User.hpp"
#include "Channel.hpp"

void ircserv(int port, std::string serverPassword)
{
	Server myServ(port, serverPassword);

	myServ.bind_socket_to_address();
	myServ.start_listening();
	myServ.run();
}

int main(int argc, char** argv)
{
	try
	{
		if (argc == 3 && check_port(argv[1]) && check_password(argv[2]))
			ircserv(atoi(argv[1]), argv[2]);
		else
			throw (std::invalid_argument("Error : usage ./ircserv <PORT> <PASSWORD>"));
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return (1);
	}
	return (0);
}
