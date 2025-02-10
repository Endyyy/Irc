#ifndef TOOLS_HPP
# define TOOLS_HPP

# define PORT_MIN 1024
# define PORT_MAX 65536
# define LEN_PWD_MIN 1
# define LEN_PWD_MAX 32
# define LEN_PORT_MAX 5

# include <climits>
# include <cstdlib>
# include <cstring>
# include <iostream>

typedef int type_sock;

bool	check_password(std::string arg);
bool	check_port(const char* arg);
bool	check_valid_limit(std::string arg, int max);
bool	have_whitespaces(std::string arg);

#endif
