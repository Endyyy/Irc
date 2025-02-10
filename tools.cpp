#include "tools.hpp"

bool	check_password(std::string arg)
{
	if (arg.empty() || arg.size() < LEN_PWD_MIN || arg.size() > LEN_PORT_MAX)
		return (false);

	if (!have_whitespaces(arg))
		return (false);

	return (true);
}

bool	check_port(const char* arg)
{
	std::string copy = arg;
	char* endptr;
	long value = 0;

	if (copy.empty() || copy.size() > LEN_PORT_MAX)
		return (false);

	value = std::strtol(arg, &endptr, 10);
	if (*endptr != '\0' || arg[0] == '+')
		return (false);

	if (value < PORT_MIN || value > PORT_MAX)
		return (false);
	return (true);
}

bool	check_valid_limit(std::string arg, int max)
{
	long unsigned int size = 0;
	int checker = max;
	while (checker)
	{
		checker = checker / 10;
		size++;
	}
	if (arg.size() > size)
		return (false);

	for (int i = 0; arg[i]; i++)
		if (strchr("0123456789", arg[i]) == NULL)
			return (false);

	if (atoi(arg.c_str()) > max)
		return (false);

	return (true);
}

bool	have_whitespaces(std::string arg)
{
	for (long unsigned int i = 0; i < arg.size(); i++)
	{
		if (isspace(arg[i]))
			return (false);
		if (i == INT_MAX)
			return (false);
	}
	return (true);
}
